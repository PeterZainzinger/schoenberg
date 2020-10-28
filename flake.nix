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
            config = mkOption rec {
              type = types.attrs;
              apply = recursiveUpdate default;
              default = {
                mapping = { };
                layers = [ ];
              };
            };
          };
          config = mkIf cfg.enable {
            environment.systemPackages = with pkgs; [
              interception-tools
              self.defaultPackage.aarch64-linux
            ];
            systemd.services."schoenberg" = {
              enable = true;
              description = "Keyboard Mapping";

              path = [
                pkgs.interception-tools
                self.defaultPackage.aarch64-linux
                pkgs.bash
              ];
              serviceConfig = {
                ExecStart = "${pkgs.interception-tools}/bin/udevmon -c /etc/schoenberg_udev.yaml";
              };

              wants = [ "systemd-udev-settle.service" ];
              after = [ "systemd-udev-settle.service" ];
              wantedBy = [ "multi-user.target" ];
            };
            environment.etc."schoenberg_config.json" =
              {
                mode = "004";
                text = builtins.toJSON cfg.config;
              };
            environment.etc."schoenberg_udev.yaml" =
              {
                mode = "004";
                text =
                  ''
                    - JOB: "${pkgs.interception-tools}/bin/intercept -g $DEVNODE | ${self.defaultPackage.aarch64-linux}/bin/schoenberg_run /etc/schoenberg_config.json | ${pkgs.interception-tools}/bin/uinput -d $DEVNODE"
                      DEVICE:
                        EVENTS:
                          EV_KEY: [KEY_F]
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
              which
              cmake
              gcc
              jetbrains.clion
            ]) ++ commonPkgs;
            shellHook = ''
            '';
          };

      }));
}
