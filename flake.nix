{
  description = "schoenberg";
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils }:
    ({
      nixosModule = { config, pkgs, lib, ... }:
        with lib;
        let
          cfg = config.services.schoenberg;
        in
        {
          options.services.schoenberg = {
            enable = mkEnableOption "schoenberg";
          };
          config = mkIf cfg.enable {
            environment.systemPackages = with pkgs; [
              interception-tools
              self.defaultPackage.aarch64-linux
            ];
            environment.etc."schoenberg_config.yaml" =
              {
                mode = "004";
                text =
                  ''
                    mapping:
                      ESC: CAPSLOCK
                      CAPSLOCK: ESC
                    layers:
                      - name: right hand
                        prefix: F
                        keys:
                          J: DOWN
                          K: UP
                          H: LEFT
                          L: RIGHT
                  '';
              };
            environment.etc."schoenberg_udev.yaml" =
              {
                mode = "004";
                text =
                  ''
                    - JOB: "${pkgs.interception-tools}/bin/intercept -g $DEVNODE | ${self.defaultPackage.aarch64-linux}/bin/schoenberg_run /etc/schoenberg_config.yaml | ${pkgs.interception-tools}/bin/uinput -d $DEVNODE"
                      DEVICE:
                        EVENTS:
                          EV_KEY: [KEY_S]
                  '';
              };

          };
        };

    } //
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = (import nixpkgs {
          system = system;
        });
        commonPkgs = with pkgs; [
          gtest
          libyamlcpp
          libevdev
          nlohmann_json
          interception-tools
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

      }));
}
