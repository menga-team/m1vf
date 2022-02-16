use std::{
    collections::HashMap,
    fmt::Display,
    fs::{self, DirEntry, File},
    hash::Hash,
    io::{stdout, ErrorKind, Write, stderr},
    path::PathBuf,
    process::Command,
    vec,
};

use byteorder::{WriteBytesExt, BE};
use clap::Parser;

use anyhow::anyhow;
use colored::Colorize;

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    #[clap(short, long)]
    input: PathBuf,
    #[clap(short, long)]
    output: PathBuf,
    #[clap(short = 'r', long, default_value_t = 30)]
    framerate: u8,
    #[clap(short, long)]
    width: u16,
    #[clap(short, long)]
    height: u16,
}
type ENDIANESS = BE;
const THREASHHOLD: u8 = 127;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u8)]
enum Algorithm {
    Uncompressed = 0x0,
    Same = 0x1,
    RunLengthRows = 0x2,
    RunLengthColumns = 0x3,
    PartialRowsUncompressed = 0x4,
    PartialColumnsUncompressed = 0x5,
}

const ALGORITHMS: [Algorithm; 3] = [
    Algorithm::Uncompressed,
    Algorithm::Same,
    Algorithm::RunLengthRows,
];

impl Algorithm {
    pub fn encode(&self, current_frame: &Vec<u8>, last_frame: &Vec<u8>) -> anyhow::Result<Vec<u8>> {
        match self {
            Algorithm::Uncompressed => uncompressed(&current_frame),
            &Algorithm::Same => {
                let mut same = true;
                for i in 0..current_frame.len() {
                    if current_frame[i] != last_frame[i] {
                        same = false;
                        break;
                    }
                }
                if same {
                    Ok(Vec::with_capacity(0))
                } else {
                    Err(anyhow!("Not the same"))
                }
            }
            &Algorithm::RunLengthRows => run_length_rows(current_frame),
            _ => {
                unreachable!()
            }
        }
    }

    pub fn name(&self) -> String {
        String::from(match self {
            Algorithm::Uncompressed => "1bpp",
            Algorithm::Same => "Frame repeat",
            Algorithm::RunLengthRows => "Run-length Row",
            Algorithm::RunLengthColumns => "Run-length Column",
            Algorithm::PartialRowsUncompressed => "Partial Rows 1bpp",
            Algorithm::PartialColumnsUncompressed => "Partial Rows 1bpp",
        })
    }
}

fn main() -> anyhow::Result<()> {
    let args = Args::parse();
    if args.input.is_file() {
        let err = fs::create_dir("frames");
        match fs::create_dir("frames") {
            Ok(()) => {}
            Err(e) => {
                if e.kind() == ErrorKind::AlreadyExists {
                    fs::remove_dir_all("frames")?;
                    fs::create_dir("frames")?;
                    println!("{}", "`frames` already existed, deleted.".yellow());
                }
            }
        }
        println!("extracting frames via ffmpeg...");
        let command = Command::new("ffmpeg")
            .arg("-i")
            .arg((&args.input).to_str().ok_or(anyhow!("Invalid file path"))?)
            .arg("-vf")
            .arg(format!(
                "scale={}:{}:flags=neighbor",
                args.width, args.height
            ))
            .arg("-r")
            .arg(format!("{}", args.framerate))
            .arg("frames/%09d.bmp")
            .output()?;
    }
    let contents = fs::read_dir(if args.input.is_dir() {
        args.input
    } else {
        PathBuf::from("frames")
    })?;
    let contents: Vec<DirEntry> = contents.map(|entry| entry.unwrap()).collect();
    let mut output = File::create(args.output)?;
    let mut last_pixels = vec![0u8; (args.width as usize * args.height as usize) as usize];
    // used for statistics for how many times a certian algorithm is used
    let mut statistics: HashMap<Algorithm, u32> = HashMap::new();
    for algo in ALGORITHMS {
        statistics.insert(algo, 0);
    }
    let mut compressed_frames = Vec::with_capacity(contents.len());
    let mut progress = 0;
    for frame in &contents {
        let image = image::open(frame.path())?;
        let image = image.grayscale();
        let image = image.as_luma8().ok_or(anyhow!("Unable to get as luma8"))?;
        //dither(&mut image, &BiLevel);
        let pixels: Vec<u8> = image
            .pixels()
            .into_iter()
            .map(|p| if p.0[0] > THREASHHOLD { 1 } else { 0 })
            .collect();
        let mut algos: Vec<(Algorithm, Vec<u8>)> = Vec::with_capacity(ALGORITHMS.len());
        //eprintln!("FRAME {}", progress);
        for algo in ALGORITHMS {
            let encoded = algo.encode(&pixels, &last_pixels);
            if encoded.is_ok() {
                algos.push((algo, encoded?));
            }
        }
        let mut smallest = &algos[0];
        for possible_algo in &algos {
            if possible_algo.1.len() < smallest.1.len() {
                smallest = possible_algo;
            }
        }
        let count = statistics.get_mut(&smallest.0).unwrap();
        *count += 1;
        compressed_frames.push(smallest.clone());
        last_pixels = pixels;
        progress += 1;
        print!("\rExtracting frames:\t{}/{}... ", progress, contents.len());
        stdout().flush().unwrap();
    }
    println!("{}", "[done]".green());
    output.write_u16::<ENDIANESS>(args.width)?;
    output.write_u16::<ENDIANESS>(args.height)?;
    output.write_u8(args.framerate)?;
    let mut last_algorithm: Option<Algorithm> = None;
    let mut algorithm_count = 0;
    for compressed_frame_index in 0..compressed_frames.len() {
        let compressed_frame = &compressed_frames[compressed_frame_index];
        if last_algorithm.is_none() {
            last_algorithm = Some(compressed_frame.0);
        } else if last_algorithm.unwrap() == compressed_frame.0 {
            algorithm_count += 1;
        }
        // println!(
        //     "{:6}: {:?} (CURRENT: {:?}) ({})",
        //     compressed_frame_index, last_algorithm, compressed_frame.0, algorithm_count
        // );
        if algorithm_count == 31 { // new spec
            // println!("write");
            output.write_all(&encode_algos(
                &compressed_frames,
                algorithm_count,
                compressed_frame_index,
                last_algorithm.unwrap(),
            )?)?;
            algorithm_count = 0;
            last_algorithm = None;
        } else if last_algorithm.unwrap() != compressed_frame.0 {
            // println!("write change");
            output.write_all(&encode_algos(
                &compressed_frames,
                algorithm_count,
                compressed_frame_index - 1,
                last_algorithm.unwrap(),
            )?)?;
            algorithm_count = 0;
            last_algorithm = Some(compressed_frame.0);
        }
        print!("\rEncoding frames:\t{}/{}... ", progress, contents.len());
        stdout().flush().unwrap();
    }
    println!("{}", "[done]".green());
    output.write_all(&encode_algos(
        &compressed_frames,
        algorithm_count,
        compressed_frames.len() - 1,
        last_algorithm.unwrap(),
    )?)?;
    println!("done.");
    println!("{:20} {}", "NAME", "COUNT");
    for (algo, count) in statistics {
        println!("{:20} {}", algo.name(), count);
    }
    Ok(())
}

fn encode_algos(
    compressed_frames: &Vec<(Algorithm, Vec<u8>)>,
    algorithm_count: u8,
    compressed_frame_index: usize,
    last_algorithm: Algorithm,
) -> anyhow::Result<Vec<u8>> {
    let mut buf = Vec::new();
    buf.write_u8(((last_algorithm as u8) << 5) | (algorithm_count))?; // new spec
    // println!(
    //     "{:08b}: {}..={}",
    //     (((last_algorithm as u8) << 4) | (algorithm_count)),
    //     (compressed_frame_index - (algorithm_count) as usize),
    //     compressed_frame_index
    // );
    for i in (compressed_frame_index - (algorithm_count) as usize)..=compressed_frame_index {
        let (_, frame) = &compressed_frames[i];
        buf.write_all(&frame)?;
    }
    Ok(buf)
}

fn uncompressed(pixels: &Vec<u8>) -> anyhow::Result<Vec<u8>> {
    let mut buf = Vec::new();
    let mut index = 7;
    let mut cbyte = 0;
    for pixel in pixels {
        cbyte |= pixel << index;
        if index == 0 {
            index = 7;
            buf.write_u8(cbyte)?;
            cbyte = 0;
        } else {
            index -= 1;
        }
    }
    Ok(buf)
}

fn run_length_rows(pixels: &Vec<u8>) -> anyhow::Result<Vec<u8>> {
    let mut buf = Vec::new();
    let mut color: Option<u8> = None;
    let mut repeat: u8 = 0;
    fn encode_byte(color: u8, repeat: u8) -> u8 {
        //eprintln!("{:08b}, rep: {}", color << 7, repeat);
        (color << 7) | repeat
    }
    for pixel in pixels {
        //let pixel = &pixels[pixel_index];
        //eprintln!("{}", pixel);
        if color.is_none() {
            repeat = 0;
            color = Some(*pixel);
        } else if *pixel == color.unwrap() {
            repeat += 1;
        }
        //eprintln!("C: {} - {}", color.unwrap(), repeat);
        if *pixel != color.unwrap() || repeat == 0b0111_1111 {
            buf.write_u8(encode_byte(color.unwrap(), repeat))?;
            repeat = 0;
            color = Some(*pixel);
        }
    }
    if color.is_some() {
        buf.write_u8(encode_byte(color.unwrap(), repeat))?;
    }
    Ok(buf)
}
