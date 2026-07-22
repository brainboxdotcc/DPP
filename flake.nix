{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      perSystem =
        { pkgs, ... }:
        {
          devShells.default = pkgs.mkShell.override { stdenv = pkgs.llvmPackages.libcxxStdenv; } {
            packages = with pkgs; [
              llvmPackages.clang-tools
              llvmPackages.lldb
              cmake
              cmake-language-server
              ninja
              pkg-config
              zlib
              openssl
              opus
            ];

            CMAKE_EXPORT_COMPILE_COMMANDS = "1";
            CMAKE_GENERATOR = "Ninja";
            CC = "clang";
            CXX = "clang++";
          };
        };
    };
}
