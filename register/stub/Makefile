inc_path := $(shell pwd)/metadef/inc/external/
out_path := $(shell pwd)/out/register/lib64/stub/
stub_path := $(shell pwd)/metadef/register/stub/

mkdir_stub := $(shell mkdir -p $(out_path))
register_local_stub := $(shell $(HI_PYTHON) $(stub_path)/gen_stubapi.py $(inc_path) $(out_path))
