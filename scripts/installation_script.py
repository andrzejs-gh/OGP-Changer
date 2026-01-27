import sys
import os
import subprocess
import shutil
import tempfile
from utils import *

def check_for_cmake_n_build_essentials():
	if path := shutil.which("cmake"):
		print(f"Found cmake at: {path} {ok}")
	else:
		raise Exception("CMake not found. You must install it in order to build the binaries.\n"
						"Debian / Ubuntu:\n sudo apt install cmake\n\n"
						"Fedora:\n sudo dnf install cmake\n\n"
						"Arch:\n sudo pacman -S cmake\n\n")
	
	backend_found, compiler_found = False, False
	for bin_name in ["g++", "clang++", "make", "ninja"]:
		if path := shutil.which(bin_name):
			print(f"Found {bin_name} at: {path} {ok}")
			if bin_name == "g++" or bin_name == "clang++":
				compiler_found = True
			else:
				backend_found = True
	
	if not (backend_found and compiler_found):
		raise Exception("Build backend or compiler not found. "
						"You must install it in order to build the binaries.\n"
						"Debian / Ubuntu:\n" 
						"sudo apt install build-essential\n\n"
						"Fedora:\n" 
						"sudo dnf group install development-tools\n\n"
						"Arch:\n" 
						"sudo pacman -S base-devel\n\n")

def prepare_build_dir() -> str:
	temp_dir = "/dev/shm"
	
	if not (is_accessible(temp_dir)):
		temp_dir = None
	build_dir = tempfile.mkdtemp(prefix="ogp_changer_build-", dir=temp_dir)
	
	if build_dir:
		print(f"Build directory mounted at: {build_dir} {ok}")
		
	return build_dir

def build_binaries(build_dir):
	result = subprocess.run(["cmake", "-S", src_dir, "-B", build_dir])
	if result.returncode != 0:
		raise Exception("CMake configuration failed.")
	result = subprocess.run(["cmake", "--build", build_dir])
	if result.returncode != 0:
		raise Exception("CMake build failed.")
		
	print(f"Binaries successfully built {ok}")
	
def install(build_dir):
	home_dir = get_home_dir()
	home_bin_dir = home_dir + "/.local/bin"
	
	os.makedirs(home_bin_dir, exist_ok=True)
	if not is_accessible(home_bin_dir):
		raise Exception(f"{home_bin_dir} is not writable.")
	
	for binary_name in ["cogp", "ogp_changer"]:
		path = home_bin_dir+ "/" + binary_name
		if os.path.exists(path):
			try:
				os.remove(path)
			except Exception as e:
				raise Exception(f"Could not remove pre-existing {path}:\n{e}")
		
		path = build_dir + "/" + binary_name
		try:
			shutil.move(path, home_bin_dir)
			print(f"Installed {binary_name} in {home_bin_dir} {ok}")
		except Exception as e:
			raise Exception(f"Failed to move {path} to {home_bin_dir}:\n{e}")
	
	kfileitemaction_dir = get_system_plugin_dir() + "/kf6/kfileitemaction"
	print(blue("\n*** PLUGIN INSTALLATION ***\n"))
	print(f"To install the plugin in:\n" 
		  f"{kfileitemaction_dir}\n\n"
		  "root privileges are required.\n\n"
		  f"If you prefer, you can manually move the plugin there from:\n" 
		  f"{build_dir}/ogp_changer_fileitemaction.so \n\n"
		  f"However, you must do it now before you skip this step, otherwise the temp directory:\n"
		  f"{build_dir}\n\n"
		  "will be deleted.\n\n"
		  "Do you want to install the plugin automatically? (root required)?\n\n" 
		  "[y - yes / <any other input> - no]")
	if input().lower() == "y":
		script_path = this_dir + "/install_so.py"
		result = subprocess.run(["sudo", "python3", script_path, build_dir, kfileitemaction_dir])
		if result.returncode != 0:
			raise Exception("Plugin installation failed.")
	else:
		print(f"Plugin auto installation canceled. {ok}")
		
	print(f"Instalation successfull. {ok}")

def clean_build_dir(build_dir):
	if not build_dir:
		return
	try:
		shutil.rmtree(build_dir)
	except Exception as e:
		print(red("Build directory clean-up failed:"))
		print(e)

if __name__ == "__main__":
	try:
		build_dir = ""
		check_for_cmake_n_build_essentials()
		build_dir = prepare_build_dir()
		build_binaries(build_dir)
		install(build_dir)
		clean_build_dir(build_dir)
		print(green("Installation finished\n"))
		print(blue("=== IMPORTANT ==="))
		print("For the plugin to start working you might need to log out and log in again.")
		input("Press ENTER to exit.")
		sys.exit(0)
	except Exception as e:
		print(error)
		print(e)
		clean_build_dir(build_dir)
		input("Press ENTER to exit.")
		sys.exit(1)

