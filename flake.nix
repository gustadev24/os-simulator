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
        in
        pkgs.mkShell {
          packages = (
            with pkgs;
            [
              gcc
              cmake
              just
              graphviz
              doxygen
              texlive.combined.scheme-full
              (python312.withPackages (
                ps: with ps; [
                  matplotlib
                  numpy
                  pytest
                  pandas
                  seaborn
                  plotutils
                  click
                ]
              ))
            ]
          );
          buildInputs = [ pkgs.bashInteractive ];
          shellHook = '''';
        };
    };
}
