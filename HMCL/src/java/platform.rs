use windows::Win32::System::SystemInformation::{
    GetNativeSystemInfo, SYSTEM_INFO, PROCESSOR_ARCHITECTURE_ARM64,
    PROCESSOR_ARCHITECTURE_AMD64, PROCESSOR_ARCHITECTURE_INTEL,
};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Architecture {
    X86,
    X86_64,
    Arm64,
}

pub struct Platform {
    pub architecture: Architecture,
}

impl Platform {
    pub fn current() -> Self {
        let architecture = Self::detect_architecture();

        Self {
            architecture,
        }
    }

    fn detect_architecture() -> Architecture {
        let mut system_info = SYSTEM_INFO::default();
        unsafe { GetNativeSystemInfo(&mut system_info) };

        unsafe {
            match system_info.Anonymous.Anonymous.wProcessorArchitecture {
                PROCESSOR_ARCHITECTURE_ARM64 => Architecture::Arm64,
                PROCESSOR_ARCHITECTURE_AMD64 => Architecture::X86_64,
                PROCESSOR_ARCHITECTURE_INTEL => Architecture::X86,
                _ => Architecture::X86,
            }
        }
    }
}