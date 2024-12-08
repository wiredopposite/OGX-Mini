# This script copies BTstack ESP32 port files to this project's components folder, ESP-IDF freaks out otherwise

import os
import sys
import shutil

def main(btstack_root, components_dir):
	PROJECT_COMPONENTS = os.path.abspath(components_dir)
	BTSTACK_DIR = os.path.abspath(btstack_root)
	BTSTACK_PORT_DIR = os.path.abspath(os.path.join(BTSTACK_DIR, 'port', 'esp32'))

	if not os.path.exists(PROJECT_COMPONENTS):
		print(f"Error: No components folder at {PROJECT_COMPONENTS}")
		sys.exit(10)
		
	if not os.path.exists(BTSTACK_DIR):
		print(f"Error: No BTstack folder at {BTSTACK_DIR}")
		sys.exit(10)

	PROJECT_BTSTACK = os.path.join(PROJECT_COMPONENTS, "btstack")

	if os.path.exists(PROJECT_BTSTACK):
		print("Deleting old BTstack component %s" % PROJECT_BTSTACK)
		shutil.rmtree(PROJECT_BTSTACK)

	# create components/btstack
	print("Creating BTstack component at %s" % PROJECT_COMPONENTS)
	shutil.copytree(os.path.join(BTSTACK_PORT_DIR, 'components', 'btstack'), PROJECT_BTSTACK)

	dirs_to_copy = [
		'src',
		'3rd-party/bluedroid',
		'3rd-party/hxcmod-player',
		'3rd-party/lwip/dhcp-server',
		'3rd-party/lc3-google',
		'3rd-party/md5',
		'3rd-party/micro-ecc',
		'3rd-party/yxml',
		'platform/freertos',
		'platform/lwip',
		'tool'
	]

	for dir in dirs_to_copy:
		print('- %s' % dir)
		shutil.copytree(os.path.join(BTSTACK_PORT_DIR, '..', '..', dir), os.path.join(PROJECT_BTSTACK, dir))

	# manually prepare platform/embedded
	print('- platform/embedded')
	platform_embedded_path = PROJECT_BTSTACK + '/platform/embedded'
	os.makedirs(platform_embedded_path)
	platform_embedded_files_to_copy = [
		'hal_time_ms.h',
		'hal_uart_dma.h',
		'hci_dump_embedded_stdout.h',
		'hci_dump_embedded_stdout.c',
	]
	for file in platform_embedded_files_to_copy:
		shutil.copy(os.path.join(BTSTACK_PORT_DIR, '..', '..', 'platform', 'embedded', file), platform_embedded_path)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: script.py <path_to_btstack> <path_to_project_components>")
        sys.exit(1)

    btstack_root = sys.argv[1]
    project_components = sys.argv[2]

    if not os.path.exists(btstack_root):
        print(f"Error: Specified external directory does not exist: {btstack_root}")
        sys.exit(2)

    if not os.path.exists(project_components):
        print(f"Error: Specified components directory does not exist: {project_components}")
        sys.exit(3)

    main(btstack_root, project_components)