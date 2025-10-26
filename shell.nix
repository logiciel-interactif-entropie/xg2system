let
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-25.05";
  pkgs = import nixpkgs { config = {}; overlays = []; };
  bgfx = pkgs.clangStdenv.mkDerivation {
    name = "bgfx";
    src = pkgs.fetchFromGitHub {
      owner = "bkaradzic";
      repo = "bgfx.cmake";
      rev = "5f3f4f29726dbfa5e0de2a05e9daea1a89700c8d";
	    hash = "sha256-O2+nSK74aXWTQ5QSBTqSeYBo4ikg/TTHzAeg6OYDJVM=";
	    fetchSubmodules = true;
    };
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    buildInputs = [ pkgs.xorg.libX11 pkgs.libGL pkgs.wayland pkgs.spirv-tools pkgs.spirv-cross pkgs.spirv-headers ];
    configurePhase = ''
        mkdir -p build
        cd build
        cmake ../ -DCMAKE_INSTALL_PREFIX=$out/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2" -GNinja
      '';
    buildPhase = ''
        cmake --build . --parallel $(nproc) --config Release
      '';
  };
  mkEnvironment = pkgs: name: pkgs.llvmPackages_latest.stdenv.mkDerivation {
    name = name;
    nativeBuildInputs = [ pkgs.llvmPackages_latest.clang-tools pkgs.meson pkgs.ninja pkgs.gdb pkgs.cmake pkgs.pkg-config ];
    buildInputs = [ pkgs.glib pkgs.cglm pkgs.glfw pkgs.libGL pkgs.xorg.libX11 pkgs.xorg.libXrandr pkgs.assimp bgfx ];
  };
in
{
  native = mkEnvironment pkgs "xg2system";
}
  
