import glob
import subprocess
from pathlib import Path


def main() -> None:
    for proto_file in glob.glob("grpc_protos/*.proto"):
        target_dir = Path("grpc_gen") / Path(proto_file).stem
        print(f"Generating models for {proto_file} in {target_dir}")

        target_dir.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [
                "protoc",
                "-I",
                ".",
                "--python_betterproto_opt=pydantic_dataclasses",
                f"--python_betterproto_out={target_dir}",
                proto_file,
            ],
            check=False,
        )


if __name__ == "__main__":
    main()
