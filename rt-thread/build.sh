#!/bin/bash

export TOP_DIR_LITE=`pwd`
export CPU_DIR_LITE="${TOP_DIR_LITE}/libcpu/risc-v/spacemit"
export BSP_DIR_LITE="${TOP_DIR_LITE}/bsp/spacemit"
export BOARD_DIR_LITE="${BSP_DIR_LITE}/platform"
export ESOS_LITE_BASE_DEFCONF="${BSP_DIR_LITE}/.esos.config"
export ESOS_LITE_DEFCONF="${BSP_DIR_LITE}/.config"
export ESOS_LITE_BINARY_OUTPUT="${TOP_DIR_LITE}/../../../bsp/spacemit/binary"

TARGET_CHIP_LITE=
TARGET_ENTRY_POINT_LITE=
TARGET_DEFCONFIG_LITE=

function mk_error()
{
	echo -e "\033[40;31mERROR: $*\033[0m"
}

function mk_warn()
{
	echo -e "\033[40;33;1mmWARN: $*\033[0m"
}

function mk_info()
{
	echo -e "\033[40;37mINFO: $*\033[0m"
}

function select_chip()
{
	count=0

	printf "All valid soc chips:\n"

	for chip in $(cd $CPU_DIR_LITE/; find -mindepth 1 -maxdepth 1 -type d |sort); do
		if [ `basename $CPU_DIR_LITE/$TARGET_CHIP_LITE/$chip` != ".git" ] ; then
			chips[$count]=`basename $CPU_DIR_LITE/$chip`
			printf "	$count: ${chips[$count]}\n"
			let count=$count+1
		fi
	done

	if [ "$count" -gt 0 ] ; then
		while true; do
			read -p "Please select a chip:"
			RES=`expr match $REPLY "[0-9][0-9]*$"`
			if [ "$RES" -le 0 ]; then
				printf "please use index number\n"
				continue
			fi
			if [ "$REPLY" -ge $count ] || [ "$REPLY" -lt "0" ]; then
				mk_error "input is invalid!"
				continue
			fi
			break
		done

		TARGET_CHIP_LITE=${chips[$REPLY]}
		return 0
	else
		mk_error "No valid chip!"
		return 1
	fi
}

function select_entry_point()
{
	if [ "x${TARGET_CHIP_LITE}" = "xrt24" ]; then
		TARGET_ENTRY_POINT_LITE=0x0
	else
		mk_error "No valid entry point!"
		return 1
	fi
}

# build script usage helper
function build_usage()
{
	CMD_PROMPT="./build.sh"
	mk_info "usage of build script is as follows:
	'$CMD_PROMPT config'                 set the SDK configuration
	'$CMD_PROMPT all'                    build all component
	'$CMD_PROMPT'                        build all component
	'$CMD_PROMPT clean'                  clean the kernel\n"
}

# show target configuration
function show_target_config()
{
	echo
	mk_info "target configuration is as follows:"
	mk_info "-------------------------------------------------------------------------"
	cat ${ESOS_LITE_BASE_DEFCONF}
	mk_info "-------------------------------------------------------------------------"
}

function config_sdk()
{
	mk_info "prepare to config sdk ..."

	# delete the old configuration script
	rm -rf ${ESOS_LITE_DEFCONF}
	rm -rf ${ESOS_LITE_BASE_DEFCONF}

	select_chip
	select_entry_point

	TARGET_DEFCONFIG_LITE=${TARGET_CHIP_LITE}_defconfig

	# check if the build.cfg is full configured
	if [ "x${TARGET_CHIP_LITE}" = "x" ]; then
		mk_error "TARGET_CHIP_LITE is not configured!!!"
	fi

	echo "export TARGET_CHIP_LITE=${TARGET_CHIP_LITE}" >> ${ESOS_LITE_BASE_DEFCONF}
	echo "export TARGET_DEFCONFIG_LITE=${TARGET_DEFCONFIG_LITE}" >> ${ESOS_LITE_BASE_DEFCONF}
	echo "export TARGET_ENTRY_POINT_LITE=${TARGET_ENTRY_POINT_LITE}" >> ${ESOS_LITE_BASE_DEFCONF}

	source ${ESOS_LITE_BASE_DEFCONF}

	cp ${BOARD_DIR_LITE}/${TARGET_CHIP_LITE}/${TARGET_DEFCONFIG_LITE} ${BSP_DIR_LITE}/.config

	show_target_config
}

function build_kernel()
{
	# build src 
	source ${ESOS_LITE_BASE_DEFCONF}

	export TARGET_CHIP_LITE TARGET_ENTRY_POINT_LITE

	cd ${BSP_DIR_LITE}
	scons --useconfig=.config
	scons

	# copy the binary
	if [ ! -d "${ESOS_LITE_BINARY_OUTPUT}" ]; then
		mkdir -p ${ESOS_LITE_BINARY_OUTPUT}
	fi

	mv ${BSP_DIR_LITE}/esos_lite* ${ESOS_LITE_BINARY_OUTPUT}

	# clean the kernel
	scons -c

	cd -
}

function clean_kernel()
{
	# clean src
	source ${ESOS_LITE_BASE_DEFCONF}
	cd ${BSP_DIR_LITE}
	scons -c
	cd -
}

function kernel_menuconfig()
{
	# build src
	source ${ESOS_LITE_BASE_DEFCONF}
	# Export variables for Python scripts
	export TARGET_CHIP_LITE TARGET_ENTRY_POINT_LITE ESOS_LITE_DEFCONF
	cd ${BSP_DIR_LITE}
	scons --useconfig=.config
	if [ $? -ne 0 ]; then
		mk_error "Failed to load config"
		cd -
		return 1
	fi

	# Calculate MD5 before menuconfig
	if [ -f .config ]; then
		CUR_MD5=`md5sum .config`
	else
		CUR_MD5=""
	fi

	scons --menuconfig
	if [ $? -ne 0 ]; then
		mk_error "Failed to build esos"
		cd -
		return 1
	fi
	
	# Calculate MD5 after menuconfig
	if [ -f .config ]; then
		NEW_MD5=`md5sum .config`
	else
		NEW_MD5=""
	fi

	# If config changed, save back to defconfig
	if [ "${CUR_MD5}" != "${NEW_MD5}" ]; then
		mk_info "Configuration changed, saving to defconfig..."

		# Save full .config format (preserves all options and comments)
		if [ -d "platform/${TARGET_CHIP_LITE}" ]; then
			cp .config platform/${TARGET_CHIP_LITE}/${TARGET_DEFCONFIG_LITE}
			mk_info "Updated platform/${TARGET_CHIP_LITE}/${TARGET_DEFCONFIG_LITE}"
		fi
	else
		mk_info "Configuration not changed"
	fi

	cd -

	return 0
}
# execute some command without configuration
if [ "x$1" = "xhelp" ]; then
	build_usage
	exit 0
elif [ "x$1" = "xconfig" ]; then
	config_sdk
	exit 0
elif [ "x$1" = "xmenuconfig" ]; then
	kernel_menuconfig
	exit 0
elif [ "x$1" = "x" ]; then
	build_kernel
	exit 0
elif [ "x$1" = "xclean" ]; then
	clean_kernel
	exit 0
fi
