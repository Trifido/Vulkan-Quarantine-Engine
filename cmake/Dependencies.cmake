# Central dependency manifest for FetchContent packages.
#
# Update these revisions deliberately and validate both Debug and Release before
# merging the change. Do not use moving branches such as main or master here.

set(QE_GLFW_REVISION "3.4")
set(QE_GLM_REVISION "1.0.1")

# stb does not publish regular versioned releases. Keep it pinned to an exact
# commit so a clean build cannot change without a repository change.
set(QE_STB_REVISION "2c980bb59875b0d32144a71867fbdebb2f77cd20")

set(QE_YAML_CPP_REVISION "yaml-cpp-0.9.0")
set(QE_KTX_REVISION "v4.4.2")
