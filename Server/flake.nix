{
  description = "Node.js MQTT Server";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells = {
          default = pkgs.mkShell rec {
            packages = with pkgs; [
              nodejs_22
              nodePackages.typescript
              nodePackages.typescript-language-server
            ];
          };
        };
      }
    );
}
