
fn main() {
    let eraft_proto_file = "./proto/eraft.proto";

    tonic_build::configure()
    .build_server(true)
    .out_dir("./src")
    .compile(&[eraft_proto_file], &["."])
    .unwrap_or_else(|e| panic!("eraft protobuf compile error: {}", e));

    println!("cargo: rerun-if-changed={}", eraft_proto_file);
}