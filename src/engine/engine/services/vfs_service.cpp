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


bool VFSService::initialize(const Engine::Config::VFSParams& params)
{
	auto& cli = service<CLIService>().parser();
	std::string rootFolder;
	cli("--rootFolder") >> rootFolder;
	if (rootFolder.empty())
	{
		ENGINE_FAIL("You have to specify root folder");
		return false;
	}

	m_vfs = std::make_shared<vfspp::VirtualFileSystem>();
	for (auto& alias : params.aliases)
	{
		vfspp::IFileSystemPtr fs = nullptr;

		auto path = rootFolder + alias.relativePath.data();
		switch (alias.type)
		{
		case Engine::Config::VFSParams::Alias::Type::Native:
		{
			fs = std::make_unique<vfspp::NativeFileSystem>(path);
			break;
		}
		case Engine::Config::VFSParams::Alias::Type::BuildZip:
		{
			fs = std::make_unique<vfspp::ZipFileSystem>(path);
			break;
		}
		default:
		{
			ENGINE_FAIL("Not implemented yet alias type!");
			break;
		}
		}

		fs->Initialize();

		m_vfs->AddFileSystem(std::string(alias.alias), std::move(fs));
	}

	return true;
}


void VFSService::release()
{
	m_vfs.reset();
}

} //-- engine.
