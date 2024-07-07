namespace BSResource {
	namespace Archive2 {
		struct ClearRegistryEvent {
			RE::BSResource::Archive2::DataReader reader;
			RE::BSResource::ID nameID;
			const char* nameText;
			std::uint32_t contentsFormat;
			std::uint32_t fileCount;
		};

		template<std::uint64_t id, std::ptrdiff_t diff>
		class Index_ProcessEventHook {
		public:
			static void Install() {
				REL::Relocation<std::uintptr_t> target(REL::ID(id), diff);
				func = *(func_t*)(target.get());
				REL::safe_write(target.get(), (std::uintptr_t)ProcessHook);
			}
		private:
			using func_t = std::uint32_t(*)(RE::BSResource::Archive2::Index&, const ClearRegistryEvent&, const RE::BSTEventSource<ClearRegistryEvent>*);

			static std::uint32_t ProcessHook(RE::BSResource::Archive2::Index& a_self, const ClearRegistryEvent& a_clrRegEvt, const RE::BSTEventSource<ClearRegistryEvent>* a_evtSrc) {
				if (a_clrRegEvt.contentsFormat == 0x4C524E47 && a_self.dataFileCount >= 0xFF) {
					F4SE::stl::report_and_fail(
						fmt::format(
							"The number of general format archives exceeds 255. "
							"This will crash the game."
						));
					return 0;
				}
				return func(a_self, a_clrRegEvt, a_evtSrc);
			}

			inline static func_t func;
		};
	}
}

namespace BSTextureIndex {
	class Index {
	public:
		std::uint64_t	unk00;
		void* unk08;
		std::uint32_t	unk10;
		std::uint32_t	unk14;
		std::uint64_t	unk18;
		RE::BSTArray<RE::BSResource::Stream> streams;	// 20
		RE::BSResource::ID id[256];	// 38
	};
}

namespace BSTextureStreamer {
	class Manager {
	public:
		std::uint64_t	unk00;
		std::uint64_t	unk08;
		std::uint64_t	unk10;
		std::uint64_t	unk18;
		std::uint64_t	unk20;
		std::uint64_t	unk28[78248];
		BSTextureIndex::Index	index;
	};

	template<std::uint64_t id, std::ptrdiff_t diff>
	class Manager_ProcessEventHook {
	public:
		static void Install() {
			REL::Relocation<std::uintptr_t> target(REL::ID(id), diff);
			func = *(func_t*)(target.get());
			REL::safe_write(target.get(), (std::uintptr_t)ProcessHook);
	}
	private:
		using func_t = std::uint32_t(*)(Manager&, const BSResource::Archive2::ClearRegistryEvent&, const RE::BSTEventSource<BSResource::Archive2::ClearRegistryEvent>*);

		static std::uint32_t ProcessHook(Manager& a_self, const BSResource::Archive2::ClearRegistryEvent& a_clrRegEvt, const RE::BSTEventSource<BSResource::Archive2::ClearRegistryEvent>* a_evtSrc) {
			if (a_clrRegEvt.contentsFormat == 0x30315844 && a_clrRegEvt.fileCount > 0 && a_self.index.streams.size() >= 0xFF) {
				F4SE::stl::report_and_fail(
					fmt::format(
						"The number of texture format archives exceeds 255. "
						"This will crash the game."
					));
				return 0;
			}
			return func(a_self, a_clrRegEvt, a_evtSrc);
		}

		inline static func_t func;
};
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface * a_f4se, F4SE::PluginInfo * a_info) {
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface * a_f4se) {
	F4SE::Init(a_f4se);

	BSResource::Archive2::Index_ProcessEventHook<1567500, 0x8>::Install();
	BSTextureStreamer::Manager_ProcessEventHook<516178, 0x8>::Install();

	return true;
}
