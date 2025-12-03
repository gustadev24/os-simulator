{
  description = "A flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
    in
    {
      devShells."${system}".default =
        let
          pkgs = import nixpkgs {
            inherit system;
            config.allowUnfree = true;
          };
          python = pkgs.python312.override {
            packageOverrides = pyfinal: pyprev: {
              py2puml = pyfinal.callPackage ./nix-pkgs/py2puml.nix { };
            };
          };
        in
        pkgs.mkShell {
          packages = (
            with pkgs;
            [
              gcc
              gdb
              ninja
              cmake
              just
              plantuml
              inkscape
              imagemagick
              clang-uml
              graphviz
              doxygen
              texlive.combined.scheme-full
              (python.withPackages (
                ps: with ps; [
                  matplotlib
                  numpy
                  pytest
                  pandas
                  py2puml
                  seaborn
                  plotutils
                  click
                  pip
                ]
              ))
            ]
          );
          buildInputs = [ pkgs.bashInteractive ];
          shellHook = '''';
        };
    };
}
