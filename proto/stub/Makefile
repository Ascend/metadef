inc_path := $(shell pwd)/metadef/inc/external/
out_path := $(shell pwd)/out/opcommon/lib64/stub/
stub_path := $(shell pwd)/metadef/opcommon/stub/

mkdir_stub := $(shell mkdir -p $(out_path))
opcommon_local_stub := $(shell $(HI_PYTHON) $(stub_path)/gen_stubapi.py $(inc_path) $(out_path))
