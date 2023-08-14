inc_path := $(shell pwd)/metadef/inc/common/util/
out_path := $(shell pwd)/out/error_manager/lib64/stub/
stub_path := $(shell pwd)/metadef/error_manager/stub/

mkdir_stub := $(shell mkdir -p $(out_path))
error_manager_local_stub := $(shell $(HI_PYTHON) $(stub_path)/gen_stubapi.py $(inc_path) $(out_path))
