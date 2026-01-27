import sys, shutil, os
from utils import *

def move_so_to_system_path(build_dir, kfileitemaction_dir):
	so_build_path = build_dir + "/ogp_changer_fileitemaction.so"
	
	if not os.path.isfile(so_build_path):
		raise Exception(f"ogp_changer_fileitemaction.so not found in {build_dir}")
	
	if not is_accessible(kfileitemaction_dir):
		raise Exception(f"System directory: {kfileitemaction_dir} is not writable.")
	
	system_so_path = kfileitemaction_dir + "/ogp_changer_fileitemaction.so"
	if os.path.exists(system_so_path):
		try:
			os.remove(system_so_path)
		except Exception as e:
			raise Exception(f"Cannot remove pre-existing {system_so_path}\n{e}")
	
	shutil.move(so_build_path, kfileitemaction_dir)
	print(f"Plugin moved to {kfileitemaction_dir} {ok}")

if __name__ == "__main__":
	try:
		argv = sys.argv
		build_dir = argv[1]
		kfileitemaction_dir = argv[2]
		move_so_to_system_path(build_dir, kfileitemaction_dir)
		sys.exit(0)
	except Exception as e:
		print(error)
		print(e)
		sys.exit(1)
		
