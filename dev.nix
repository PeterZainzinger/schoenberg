with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "schoenberg";
  src = ./.;

  buildInputs = [ 
    pkgs.gtest
    pkgs.libyamlcpp
    pkgs.libevdev

  ];
  nativeBuildInputs = [
    pkgs.which
    pkgs.cmake
    pkgs.gcc
    pkgs.python3
    pkgs.jetbrains.clion
  ];
}
