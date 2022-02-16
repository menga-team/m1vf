use std::{
    any::Any,
    error::Error,
    fs::{self, DirEntry, File},
    io::{self, Cursor, Write},
    path::PathBuf,
    thread,
    time::Duration,
};

use byteorder::{BigEndian, ReadBytesExt, WriteBytesExt, BE};
use clap::Parser;

use ffmpeg::{
    format::Pixel,
    frame::Video,
    media::Type,
    software::scaling::{Context, Flags},
};
use ffmpeg_next as ffmpeg;
use image::{
    imageops::{dither, BiLevel},
    GenericImageView,
};
use indicatif::{ProgressBar, ProgressStyle};

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    #[clap(short, long)]
    dir: PathBuf,
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
const UNCOPRESSED: u8 = 0x0;
const THREASHHOLD: u8 = 127;

fn main() -> Result<(), Box<dyn Error>> {
    // let args = Args::parse();
    // ffmpeg::init()?;
    // let mut ffmpeg = ffmpeg::format::input(&args.file)?;
    // let stream = ffmpeg
    //     .streams()
    //     .best(Type::Video)
    //     .ok_or(ffmpeg::Error::StreamNotFound)?;
    // let video_stream_index = stream.index();
    // let pb = ProgressBar::new(stream.frames() as u64);
    // pb.set_style(ProgressStyle::default_bar()
    //     .template("{spinner:.grey} [{elapsed_precise}] [{wide_bar:.white/grey}] {pos}/{len}: {per_sec} ({eta})")
    //     .progress_chars("=>-"));
    // let mut decoder = stream.codec().decoder().video()?;
    // let mut scaler = Context::get(
    //     decoder.format(),
    //     decoder.width(),
    //     decoder.height(),
    //     Pixel::RGB24,
    //     args.width as u32,
    //     args.height as u32,
    //     Flags::BILINEAR,
    // )?;
    // let mut output_no = File::create(args.output)?;
    // let mut output = Vec::<u8>::new();
    // output.write_u16::<ENDIANESS>(args.width)?;
    // output.write_u16::<ENDIANESS>(args.height)?;
    // output.write_u8(args.framerate)?;
    // let mut frame_index = 0;
    // for (stream, packet) in ffmpeg.packets() {
    //     if stream.index() == video_stream_index {
    //         decoder.send_packet(&packet)?;
    //         let mut decoded = Video::empty();
    //         while decoder.receive_frame(&mut decoded).is_ok() {
    //             let mut frame = Video::empty();
    //             scaler.run(&decoded, &mut frame)?;
    //             let data = frame.data(0);
    //             save_file(&frame, frame_index).unwrap();
    //             output.write_u8(UNCOPRESSED)?;
    //             //output.write_all(&data)?;
    //             pb.inc(1);
    //             frame_index += 1;
    //         }
    //     }
    // }
    // decoder.send_eof()?;
    // pb.finish();
    // let mut cursor = Cursor::new(output);
    // let w = cursor.read_u16::<ENDIANESS>()?;
    // let h = cursor.read_u16::<ENDIANESS>()?;
    // for _ in 0..100 {
    //     let algo = cursor.read_u8()?;
    //     dbg!(algo);
    //     let mut bytes = Vec::<u8>::new();
    //     for i in 0..w * h / 8 {
    //         bytes.push(cursor.read_u8()?);
    //     }
    //     let mut hor = 0;
    //     for i in 0..w * h {
    //         let byte = bytes[(i / 8) as usize];
    //         let bit = i % 8;
    //         let on = byte << bit & 1 == 1;
    //         if (on) {
    //             print!("x");
    //         } else {
    //             print!(" ");
    //         }
    //         std::io::stdout().flush()?;
    //         hor += 1;
    //         if (hor == w) {
    //             hor = 0;
    //             print!("\n");
    //         }
    //     }
    // }
    let args = Args::parse();
    let contents = fs::read_dir(args.dir)?;
    let contents: Vec<DirEntry> = contents.map(|entry| entry.unwrap()).collect();
    let pb = ProgressBar::new(contents.len() as u64);
    pb.set_style(ProgressStyle::default_bar()
        .template("{spinner:.grey} [{elapsed_precise}] [{wide_bar:.white/grey}] {pos}/{len}: {per_sec} ({eta})")
        .progress_chars("=>-"));
    let mut output = File::create(args.output)?;
    output.write_u16::<ENDIANESS>(args.width)?;
    output.write_u16::<ENDIANESS>(args.height)?;
    output.write_u8(args.framerate)?;
    for frame in contents {
        let mut image = image::open(frame.path())?;
        let mut image = image.grayscale();
        let mut image = image.as_mut_luma8().unwrap();
        //dither(&mut image, &BiLevel);
        let mut pixels = Vec::<u8>::new();
        for pix in image.pixels() {
            let grey = pix.0[0];
            pixels.push(if grey > THREASHHOLD { 1 } else { 0 });
        }
        output.write_u8(UNCOPRESSED)?;
        let mut index = 7;
        let mut cbyte = 0;
        for pixel in pixels {
            cbyte |= pixel << index;
            if index == 0 {
                index = 7;
                output.write_u8(cbyte)?;
                cbyte = 0;
            } else {
                index -= 1;
            }
        }
        //dbg!(frame);
        pb.inc(1);
    }
    pb.finish();
    Ok(())
}
