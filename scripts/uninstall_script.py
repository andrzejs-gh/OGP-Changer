import sys, os
from utils import *

def delete_files(paths):
	for path in paths:
		if os.path.isfile(path):
			try:
				os.remove(path)
				print(f"Removed {path} {ok}")
			except Exception as e:
				print(f"Could not remove: {path} {fail}:\n{e}")
		else:
			print(f"No {os.path.basename(path)} found in {os.path.dirname(path)} {fail}")

if __name__ == "__main__":
	try:
		home_dir = get_home_dir_from_argv()
		system_plugin_dir = get_system_plugin_dir()
		
		cogp_path = home_dir + "/.local/bin/cogp"
		ogp_changer_path = home_dir + "/.local/bin/ogp_changer"
		system_so_path = system_plugin_dir + "/kf6/kfileitemaction/lib_ogp_changer.so"
		
		delete_files([cogp_path, ogp_changer_path, system_so_path])
		
		print(green("Uninstallation completed.\n"))
		sys.exit(0)
		
	except Exception as e:
		print(error)
		print(e)
		sys.exit(1)
