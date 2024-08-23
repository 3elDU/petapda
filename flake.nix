{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs
          {
            inherit system;
            overlays = [
              (self: super: {
                picotool = (super.picotool.overrideAttrs
                  (old: rec {
                    version = "2.0.0";
                    src = super.fetchFromGitHub {
                      owner = "raspberrypi";
                      repo = "picotool";
                      rev = version;
                      sha256 = "02ckyd2lwgc7fn61i1i137cmknw6lc552acl57l570xiga9hbcfg";
                    };
                  }));
                pico-sdk = (super.pico-sdk.overrideAttrs
                  (old: rec {
                    version = "2.0.0";
                    src = super.fetchFromGitHub {
                      owner = "raspberrypi";
                      repo = "pico-sdk";
                      rev = version;
                      sha256 = "0h2j9g7z87gqv59mjl9aq7g0hll0fkpz281f4pprhjxww6789abp";
                    };
                  }));
              })
            ];
          };
      in
      {
        devShells.default = pkgs.mkShell.override
          {
            stdenv = pkgs.pkgsCross.arm-embedded.stdenv_32bit;
          }
          {
            packages = with pkgs; [
              clang-tools
              cmake
            ];
            buildInputs = with pkgs; [
              newlib
            ];
            nativeBuildInputs = with pkgs; [
              pkgsi686Linux.glibc
              python3
              ninja
              libusb1
              # Those three are overlayed with never versions
              pico-sdk
              picotool
              pioasm
              openocd
            ];

            shellHook = ''
              export PICO_BOARD="waveshare_rp2040_plus_4mb"

              export PICO_TOOLCHAIN_PATH="${pkgs.gcc-arm-embedded}/bin"
              export PICOTOOL_HINT="${pkgs.picotool}/bin"
              export PIOASM_HINT="${pkgs.pioasm}/bin"
              export PICOTOOL_DIR="${pkgs.picotool}/bin"
              export PIOASM_DIR="${pkgs.pioasm}/bin"
            '';
          };
      }
    );
}
