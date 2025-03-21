#pragma once

#include <engine/engine.h>
#include <engine/services/service_manager.h>
#include <vfspp/VFS.h>

namespace engine
{

class VFSService final : public Service<VFSService>
{
public:
	using FileMode = vfspp::IFile::FileMode;

	VFSService() = default;
	~VFSService() = default;

	bool initialize(const Engine::Config::VFSParams& params);
	void release() override;

	inline std::string absolutePath(std::string_view relativePath) const
	{
		return m_vfs->AbsolutePath(relativePath);
	}

	inline vfspp::IFilePtr openFile(std::string_view relativePath, FileMode mode = FileMode::Read)
	{
		return m_vfs->OpenFile(vfspp::FileInfo(std::string(relativePath)), mode);
	}

private:
	vfspp::VirtualFileSystemPtr m_vfs;
};

} //-- engine.
