{
  description = "A flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    nixpkgsStable.url = "github:nixos/nixpkgs?ref=nixos-23.05";
  };

  outputs =
    { nixpkgs, nixpkgsStable, ... }:
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
          pkgsStable = import nixpkgsStable {
            inherit system;
            config.allowUnfree = true;
          };
        in
        pkgs.mkShell {
          packages = (with pkgs; [
            gcc
            cmake
            just
            graphviz
            doxygen
            (python312.withPackages(ps: with ps; [
              matplotlib
              numpy
              pytest
              pandas
              seaborn
              plotutils
              click
            ]))
          ]) ++ (with pkgsStable; [
            texlive.combined.scheme-full
          ]);
          buildInputs = [ pkgs.bashInteractive ];
          shellHook = '''';
        };
    };
}
