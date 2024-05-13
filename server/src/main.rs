use image::{io::Reader as ImageReader, GenericImageView};
use tungstenite::{connect, Message};
use url::Url;

fn main() {
    let img = ImageReader::open("minitiles.png")
        .unwrap()
        .decode()
        .unwrap();

    // let (w, h) = img.dimensions();

    let bytes = img
        .pixels()
        .map(|p| {
            let (_, _, p) = p;

            (p[0] as f32 * 0.299 + p[1] as f32 * 0.587 + p[2] as f32 * 0.114) as u8

            // println!("{:?}", d);
        })
        .collect::<Vec<u8>>();

    let binding = bytes.chunks(2).collect::<Vec<&[u8]>>();
    let first_chunk = binding.first().unwrap().to_vec();

    let (mut socket, response) =
        connect(Url::parse("ws://192.168.8.175:80/").unwrap()).expect("Can't connect");

    println!("Connected to the server");
    println!("Response HTTP code: {}", response.status());
    println!("Response contains the following headers:");

    for (ref header, _value) in response.headers() {
        println!("* {}", header);
    }

    // println!("First Chunk: {:?}", first_chunk);

    // let data = String::from_utf8(first_chunk).unwrap();

    // let string_data = format!("{{\"data\": {:?}}}", first_chunk);
    // println!("Sending data: {}", string_data);

    let message = Message::Text("Hello".to_string());
    println!("Sending message: {:?}", message);

    socket.send(message).unwrap();

    // loop {
    //     let msg = socket.read().expect("Error reading message");
    //     println!("Received: {}", msg);
    // }
    // socket.close(None);
}
