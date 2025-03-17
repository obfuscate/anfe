#pragma once

#include <engine/services/service_manager.h>
#include <vfspp/VFS.h>

namespace engine
{

class VFSService final : public Service<VFSService>
{
public:
	VFSService() = default;
	~VFSService() = default;

	bool initialize() override;
	void release() override;

	std::string absolutePath(std::string_view relativePath) const;

private:
	vfspp::VirtualFileSystemPtr m_vfs;
};

} //-- engine.
