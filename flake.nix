{
  description = "schoenberg";
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = (import nixpkgs {
          system = system;
        });
        commonPkgs = with pkgs; [
          pkgs.gtest
          pkgs.libyamlcpp
          pkgs.libevdev
        ];
      in
      {
        defaultPackage = pkgs.stdenv.mkDerivation {
          name = "schoenberg";
          src = ./.;
          buildInputs = commonPkgs;
          nativeBuildInputs = [
            pkgs.cmake
          ];
          installPhase = ''
            mkdir -p $out/bin
            cp src/schoenberg_run $out/bin
          '';
        };
        devShell =
          pkgs.mkShell {
            buildInputs = (with pkgs; [
              pkgs.which
              pkgs.cmake
              pkgs.gcc
            ]) ++ commonPkgs;
            shellHook = ''
            '';
          };
      });
}
