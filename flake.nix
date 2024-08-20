{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";

    pico-sdk = {
      flake = false;
      url = "https://github.com/raspberrypi/pico-sdk";
      type = "git";
      submodules = true;
    };
    freertos = {
      flake = false;
      url = "https://github.com/raspberrypi/FreeRTOS-Kernel";
      type = "git";
      submodules = true;
    };
  };

  outputs = { nixpkgs, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            pkgsi686Linux.glibc
            gcc-arm-embedded
            cmake
            python3
            ninja
            picotool
            pioasm
            libusb1
            newlib
          ];

          shellHook = ''
            export PICO_BOARD="waveshare_rp2040_plus_4mb"

            export PICO_SDK_PATH="${inputs.pico-sdk}"
            export PICO_TOOLCHAIN_PATH="${pkgs.gcc-arm-embedded}/bin"
            export PICOTOOL_HINT="${pkgs.picotool}/bin"
            export PIOASM_HINT="${pkgs.pioasm}/bin"
            export PICOTOOL_DIR="${pkgs.picotool}/bin"
            export PIOASM_DIR="${pkgs.pioasm}/bin"
            export FREERTOS_KERNEL_PATH="${inputs.freertos}"
          '';
        };
      }
    );
}
