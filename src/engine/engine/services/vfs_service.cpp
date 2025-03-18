#include <engine/services/vfs_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>

namespace engine
{

META_REGISTRATION
{
	reflection::Service<VFSService>("VFSService")
		.cli({ "--rootFolder" });
}


bool VFSService::initialize()
{
	auto& cli = service<CLIService>().parser();
	std::string rootFolder;
	cli("--rootFolder") >> rootFolder;
	if (rootFolder.empty())
	{
		ENGINE_FAIL("You have to specify root folder");
		return false;
	}

	vfspp::IFileSystemPtr rootFS = nullptr;

#if defined(DISTRIBUTION_BUILD)
	rootFS = std::make_unique<ZipFileSystem>(rootFolder + "/resources.zip");
#else
	rootFS = std::make_unique<vfspp::NativeFileSystem>(rootFolder + "/resources");
#endif

	rootFS->Initialize();

	m_vfs = std::make_shared<vfspp::VirtualFileSystem>();
	m_vfs->AddFileSystem("/", std::move(rootFS));

	return true;
}


void VFSService::release()
{
	m_vfs.reset();
}

} //-- engine.
