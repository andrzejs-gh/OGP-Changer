import subprocess, sys
from utils import *

if __name__ == "__main__":
	try:
		home_dir = get_home_dir()
		home_bin_dir = home_dir + "/.local/bin"
		system_plugin_dir = get_system_plugin_dir()
		kfileitemaction_dir = system_plugin_dir + "/kf6/kfileitemaction"
		print(blue("\n*** UNINSTALLATION ***\n\n") +
			  "Uninstallation script will delete:\n"
			  f"{home_bin_dir}/cogp\n"
			  f"{home_bin_dir}/ogp_changer\n"
			  f"{kfileitemaction_dir}/lib_ogp_changer.so\n\n"
			  "if they exist.\n\n"
			  "To delete the plugin:\n"
			  f"{kfileitemaction_dir}/lib_ogp_changer.so\n\n"
			  "root privileges are required. If you prefer, you can delete the files manually.\n\n"
			  "Do you want to delete the files automatically (root required)?\n")
		input_ = input("[y - yes / <any other input> - no]\n")
		if input_.lower() == "y":
			cmd = ["sudo", "python3", f"{this_dir}/uninstall_script.py", home_dir]
			result = subprocess.run(cmd)
			if result.returncode != 0:
				raise Exception("Uninstallation script failed.")
		else:
			print(blue("Uninstallation canceled."))
			
		input("Press ENTER to exit.")
		sys.exit(0)
		
	except Exception as e:
		print(error)
		print(e)
		input("Press ENTER to exit.")
		sys.exit(1)
