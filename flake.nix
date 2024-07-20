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

      fs = pkgs.lib.fileset;
      getExe = pkgs.lib.getExe;

      commonSoftwareBuildAttributes = {
        buildInputs = cheriotPkgs ++ [lrPkgs.uf2conv];
        installPhase = ''
          mkdir -p $out/share/
          cp build/cheriot/cheriot/release/* $out/share/
        '';
        dontFixup = true;
      };

      cheriot-rtos-tests = pkgs.stdenvNoCC.mkDerivation ({
          name = "cheriot-rtos-tests";
          src = fs.toSource {
            root = ./.;
            fileset = fs.unions [./cheriot-rtos ./scripts/elf2uf2.sh];
          };
          buildPhase = ''
            xmake config -P cheriot-rtos/tests/ --board=sonata
            xmake -P ./cheriot-rtos/tests/
            ./scripts/elf2uf2.sh build/cheriot/cheriot/release/test-suite
          '';
        }
        // commonSoftwareBuildAttributes);

      sonata-tests = pkgs.stdenvNoCC.mkDerivation ({
          name = "sonata-tests";
          src = fs.toSource {
            root = ./.;
            fileset = fs.unions [./tests ./common.lua ./cheriot-rtos];
          };
          buildPhase = "xmake -P ./tests/";
        }
        // commonSoftwareBuildAttributes);

      sonata-examples = pkgs.stdenvNoCC.mkDerivation ({
          name = "sonata-examples";
          src = fs.toSource {
            root = ./.;
            fileset = fs.unions [
              ./common.lua
              ./cheriot-rtos
              ./libraries
              ./third_party
              ./examples
            ];
          };
          buildPhase = "xmake -P ./examples/";
        }
        // commonSoftwareBuildAttributes);

      tests-runner =
        pkgs.writers.writePython3Bin "tests-runner" {
          libraries = [pkgs.python3Packages.pyserial];
        }
        ./scripts/test_runner.py;

      tests-fpga-runner = pkgs.writers.writeBashBin "tests-fpga-runner" ''
        set -e
        if [ -z "$1" ]; then
          echo "Please provide the tty device location (e.g. /dev/ttyUSB2)" \
            "as the first argument."
          exit 2
        fi
        ${getExe tests-runner} -t 30 fpga $1 --uf2-file ${sonata-tests}/share/sonata_test_suite.uf2
        ${getExe tests-runner} -t 30 fpga $1 --uf2-file ${cheriot-rtos-tests}/share/test-suite.uf2
      '';

      tests-simulator = pkgs.stdenvNoCC.mkDerivation {
        name = "tests-simulator";
        src = ./.;
        dontBuild = true;
        doCheck = true;
        buildInputs = [sonataSystemPkgs.sonata-simulator];
        SONATA_SIM_BOOT_STUB = "${sonataSystemPkgs.sonata-sim-boot-stub.out}/share/sim_boot_stub";
        checkPhase = ''
          ${getExe tests-runner} -t 30 sim --elf-file ${sonata-tests}/share/sonata_test_suite
          ${getExe tests-runner} -t 960 sim --elf-file ${cheriot-rtos-tests}/share/test-suite
        '';
        installPhase = "mkdir $out";
      };

      lint-python = pkgs.writeShellApplication {
        name = "lint-python";
        runtimeInputs = with pkgs; [
          ruff
          (python3.withPackages (pyPkg: with pyPkg; [mypy pyserial]))
        ];
        text = ''
          set +u
          case "$1" in
            check)
              ruff format --check .
              ruff check .
              mypy .
              ;;
            fix)
              ruff format .
              ruff check --fix .
              ;;
            *) echo "Available subcommands are 'check' and 'fix'.";;
          esac
        '';
      };

      lint-cpp = let
        srcGlob = "{examples/**/*.{h,hh,cc},{libraries,tests}/*.{cc,hh}}";
      in
        pkgs.writeShellApplication {
          name = "lint-cpp";
          runtimeInputs = with lrPkgs; [llvm_cheriot];
          text = ''
            set +u
            case "$1" in
              check)
                clang-format --dry-run --Werror ${srcGlob}
                rm -f tidy_fixes
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
      lint-all = pkgs.writeShellApplication {
        name = "lint-all";
        text = ''
          ${getExe pkgs.reuse} lint
          ${getExe lint-python} check
          ${getExe lint-cpp} check
        '';
      };
    in {
      formatter = pkgs.alejandra;
      devShells = rec {
        default = pkgs.mkShell {
          name = "sonata-sw";
          packages = cheriotPkgs ++ [lrPkgs.uf2conv pkgs.python3Packages.pyserial];
        };
        env-with-sim = pkgs.mkShell {
          name = "sonata-sw-with-sim";
          packages = [sonataSystemPkgs.sonata-simulator] ++ default.nativeBuildInputs;
          SONATA_SIM_BOOT_STUB = "${sonataSystemPkgs.sonata-sim-boot-stub.out}/share/sim_boot_stub";
        };
      };
      packages = {inherit sonata-examples sonata-tests cheriot-rtos-tests;};
      checks = {inherit tests-simulator;};
      apps = builtins.listToAttrs (map (program: {
        inherit (program) name;
        value = {
          type = "app";
          program = getExe program;
        };
      }) [lint-all lint-cpp lint-python tests-runner tests-fpga-runner]);
    };
  in
    flake-utils.lib.eachDefaultSystem system_outputs;
}
