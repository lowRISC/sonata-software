# Copyright lowRISC Contributors.
# SPDX-License-Identifier: Apache-2.0
{
  description = "Sonata Software";
  inputs = {
    sonata-system.url = "github:lowRISC/sonata-system";
    lowrisc-nix.follows = "sonata-system/lowrisc-nix";
    nixpkgs.follows = "lowrisc-nix/nixpkgs";
    flake-utils.follows = "lowrisc-nix/flake-utils";
    mdutils = {
      url = "git+https://codeberg.org/HU90m/mdutils.git";
      inputs.nixpkgs.follows = "nixpkgs";
    };
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
    mdutils,
  }: let
    system_outputs = system: let
      pkgs = import nixpkgs {inherit system;};
      lrPkgs = lowrisc-nix.outputs.packages.${system};
      mdutilsPkgs = mdutils.outputs.packages.${system};
      lrDoc = lowrisc-nix.lib.doc {inherit pkgs;};
      sonataSystemPkgs = sonata-system.outputs.packages.${system};
      cheriotPkgs = lowrisc-nix.outputs.devShells.${system}.cheriot.nativeBuildInputs;

      inherit (pkgs.lib) fileset getExe;

      commonSoftwareBuildAttributes = {
        buildInputs = cheriotPkgs ++ [lrPkgs.uf2conv];
        installPhase = ''
          mkdir -p $out/share/
          cp build/cheriot/cheriot/release/* $out/share/
        '';
        dontFixup = true;
      };

      sonata-automotive-demo-legacy-component = pkgs.stdenvNoCC.mkDerivation {
        name = "sonata-automotive-demo-legacy-component";
        src = fileset.toSource {
          root = ./.;
          fileset = fileset.unions [./examples/automotive ./third_party];
        };
        buildInputs = with lrPkgs; [xmake lowrisc-toolchain-gcc-rv32imcb];
        buildPhase = "xmake -P ./examples/automotive/legacy/";
        installPhase = ''
          mkdir -p $out/share/
          cp build/ilp32/rv32imc/release/automotive_demo_send_legacy $out/share
        '';
        dontFixup = true;
      };

      sonata-software-documentation = lrDoc.buildMdbookSite {
        version = "";
        pname = "sonata-software-documentation";
        src = fileset.toSource {
          root = ./.;
          fileset = lrDoc.standardMdbookFileset ./.;
        };
        buildInputs = [mdutilsPkgs.default];
      };

      sonata-tests = pkgs.stdenvNoCC.mkDerivation ({
          name = "sonata-tests";
          src = fileset.toSource {
            root = ./.;
            fileset = fileset.unions [./tests ./common.lua ./cheriot-rtos];
          };
          buildPhase = "xmake -P ./tests/";
        }
        // commonSoftwareBuildAttributes);

      sonata-examples = pkgs.stdenvNoCC.mkDerivation ({
          name = "sonata-examples";
          src = fileset.toSource {
            root = ./.;
            fileset = fileset.unions [
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

      sonata-exercises = pkgs.stdenvNoCC.mkDerivation ({
          name = "sonata-exercises";
          src = fileset.toSource {
            root = ./.;
            fileset = fileset.unions [./exercises ./common.lua ./cheriot-rtos];
          };
          buildPhase = "xmake -P ./exercises/";
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
          ruff format --check .
          ruff check .
          mypy .
        '';
      };

      lint-cpp = let
        srcGlob = "{examples/**/*.{h,cc},exercises/**/*.{hh,cc},{libraries,tests}/*.{cc,hh}}";
      in
        pkgs.writeShellApplication {
          name = "lint-cpp";
          runtimeInputs = with lrPkgs; [llvm_cheriot];
          text = ''
            shopt -s globstar
            clang-format --dry-run --Werror ${srcGlob}
            clang-tidy ${srcGlob}
            echo "No warnings outside of dependancies."
          '';
        };

      lint-all = pkgs.writeShellApplication {
        name = "lint-all";
        text = ''
          ${getExe pkgs.reuse} lint
          ${getExe pkgs.lychee} --offline --no-progress .
          ${getExe lint-python}
          ${getExe lint-cpp}
        '';
      };
    in {
      formatter = pkgs.alejandra;
      devShells = rec {
        default = pkgs.mkShell {
          name = "sonata-sw";
          packages =
            cheriotPkgs
            ++ [
              lrPkgs.uf2conv
              lrPkgs.cheriot-audit
              pkgs.python3Packages.pyserial
              mdutilsPkgs.default
            ];
        };
        env-with-sim = pkgs.mkShell {
          name = "sonata-sw-with-sim";
          packages = [sonataSystemPkgs.sonata-simulator] ++ default.nativeBuildInputs;
          SONATA_SIM_BOOT_STUB = "${sonataSystemPkgs.sonata-sim-boot-stub.out}/share/sim_boot_stub";
        };
        env-with-everything = pkgs.mkShell {
          name = "sonata-sw-with-everything";
          packages = env-with-sim.nativeBuildInputs ++ [lrPkgs.lowrisc-toolchain-gcc-rv32imcb];
        };
      };
      packages = {
        inherit
          sonata-exercises
          sonata-examples
          sonata-tests
          sonata-software-documentation
          sonata-automotive-demo-legacy-component
          ;
      };
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
