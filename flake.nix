# Copyright lowRISC Contributors.
# SPDX-License-Identifier: Apache-2.0
{
  description = "Sonata Software";
  inputs = {
    sonata-system.url = "github:lowRISC/sonata-system";
    lowrisc-nix.follows = "sonata-system/lowrisc-nix";
    nixpkgs.follows = "lowrisc-nix/nixpkgs";
    flake-utils.follows = "lowrisc-nix/flake-utils";
  };

  nixConfig = {
    extra-substituters = ["https://nix-cache.lowrisc.org/public/"];
    extra-trusted-public-keys = ["nix-cache.lowrisc.org-public-1:O6JLD0yXzaJDPiQW1meVu32JIDViuaPtGDfjlOopU7o="];
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    lowrisc-nix,
    sonata-system,
  }: let
    system_outputs = system: let
      pkgs = import nixpkgs {inherit system;};
      lrPkgs = lowrisc-nix.outputs.packages.${system};
      sonataSystemPkgs = sonata-system.outputs.packages.${system};
      cheriotPkgs = lowrisc-nix.outputs.devShells.${system}.cheriot.nativeBuildInputs;

      getExe = pkgs.lib.getExe;

      clang-lint = let
        srcGlob = "{compartments,library}/*";
      in
        pkgs.writeShellApplication {
          name = "clang-lint";
          runtimeInputs = with lrPkgs; [llvm_cheriot];
          text = ''
            set +u
            case "$1" in
              check)
                clang-format --dry-run --Werror ${srcGlob}
                clang-tidy -export-fixes=tidy_fixes -quiet ${srcGlob}
                [ ! -f tidy_fixes ] # fail if the fixes file exists
                echo "No warnings outside of dependancies."
                ;;
              fix)
                clang-format -i ${srcGlob}
                clang-tidy -fix ${srcGlob}
                ;;
              *) echo "Available subcommands are 'check' and 'fix'.";;
            esac
          '';
        };
      run-lints = pkgs.writeShellApplication {
        name = "run-lints";
        text = ''
          ${getExe pkgs.reuse} lint
          ${getExe clang-lint} check
        '';
      };
    in {
      formatter = pkgs.alejandra;
      devShells = rec {
        default = pkgs.mkShell {
          name = "sonata-sw";
          packages = cheriotPkgs ++ [lrPkgs.uf2conv];
        };
        env-with-sim = pkgs.mkShell {
          name = "sonata-sw-with-sim";
          packages =
            [sonataSystemPkgs.sonata-simulator]
            ++ default.nativeBuildInputs;
          shellHook = ''
            export SONATA_SIM_BOOT_STUB=${sonataSystemPkgs.sonata-sim-boot-stub.out}/share/sim_boot_stub
          '';
        };
      };
      apps = {
        run-lints = {
          type = "app";
          program = getExe run-lints;
        };
        clang-lint = {
          type = "app";
          program = getExe clang-lint;
        };
      };
      checks = {
        clang-checks = pkgs.stdenvNoCC.mkDerivation {
          name = "clang-checks";
          src = ./.;
          dontBuild = true;
          doCheck = true;
          checkPhase = "${getExe clang-lint} check";
          installPhase = "mkdir $out";
        };
      };
    };
  in
    flake-utils.lib.eachDefaultSystem system_outputs;
}
