use std::path::{Path, PathBuf};
use std::collections::HashSet;
use windows::core::PWSTR;
use windows::Win32::System::Registry::{HKEY, RegOpenKeyExW, RegCloseKey, RegQueryInfoKeyW, RegEnumKeyExW, RegGetValueW, HKEY_LOCAL_MACHINE, KEY_READ, KEY_WOW64_64KEY, RRF_RT_REG_SZ};
use windows::core::PCWSTR;
use std::ffi::OsString;
use std::os::windows::ffi::OsStrExt;

use super::version::JavaVersion;
use crate::debug::DebugLogger;
use crate::java::platform::{Architecture, Platform};

#[derive(Clone)]
pub struct JavaRuntime {
    pub version: JavaVersion,
    pub executable: PathBuf,
}

pub struct JavaFinder {
    expected_major_version: u16,
    found_java: Vec<JavaRuntime>,
    seen_paths: HashSet<String>,
}

impl JavaFinder {
    pub fn new(expected_major_version: u16) -> Self {
        Self {
            expected_major_version,
            found_java: Vec::new(),
            seen_paths: HashSet::new(),
        }
    }

    pub fn find_java_installations(&mut self, java_exe_name: &str) {
        let debug_logger = DebugLogger::global();
        let platform = Platform::current();

        // Search java in JAVA_HOME
        if let Ok(java_home) = std::env::var("JAVA_HOME") {
            debug_logger.log_verbose(format!("Checking JAVA_HOME: {}", java_home));
            let java_executable = Path::new(&java_home).join("bin").join(java_exe_name);
            self.try_add_java(java_executable);
        }

        if let Ok(path_env) = std::env::var("PATH") {
            debug_logger.log_verbose("Searching in PATH".to_string());
            for path_dir in path_env.split(';') {
                if !path_dir.trim().is_empty() {
                    let java_executable = Path::new(path_dir.trim()).join(java_exe_name);
                    self.try_add_java(java_executable);
                }
            }
        }

        if let Ok(program_files) = std::env::var("ProgramFiles") {
            let program_files_path = Path::new(&program_files);
            self.search_in_program_files(program_files_path, java_exe_name);
        }

        self.search_in_registry("SOFTWARE\\JavaSoft\\JDK", java_exe_name);
        self.search_in_registry("SOFTWARE\\JavaSoft\\JRE", java_exe_name);

        if let Ok(current_dir) = std::env::current_dir() {
            let hmcl_java_dir = current_dir.join(".hmcl").join("java").join(
                match platform.architecture {
                    Architecture::Arm64 => "windows-arm64",
                    Architecture::X86_64 => "windows-x86_64",
                    Architecture::X86 => "windows-x86",
                }
            );

            if hmcl_java_dir.exists() {
                self.search_in_directory(&hmcl_java_dir, java_exe_name);
            }
        }

        if let Ok(appdata) = std::env::var("APPDATA") {
            let hmcl_java_dir = Path::new(&appdata).join(".hmcl").join("java").join(
                match platform.architecture {
                    Architecture::Arm64 => "windows-arm64",
                    Architecture::X86_64 => "windows-x86_64",
                    Architecture::X86 => "windows-x86",
                }
            );

            if hmcl_java_dir.exists() {
                self.search_in_directory(&hmcl_java_dir, java_exe_name);
            }
        }
    }

    fn try_add_java(&mut self, executable: PathBuf) {
        let debug_logger = DebugLogger::global();

        if !executable.exists() {
            return;
        }

        let path_str = executable.to_string_lossy().to_string();
        if self.seen_paths.contains(&path_str) {
            debug_logger.log_verbose(format!("Ignore duplicate Java: {}", path_str));
            return;
        }

        if let Some(version) = JavaVersion::from_java_executable(executable.as_ref()) {
            if !version.is_acceptable(self.expected_major_version) {
                debug_logger.log_verbose(format!("Java {} version {} not acceptable",
                                                 path_str, version));
                return;
            }

            self.seen_paths.insert(path_str.clone());
            self.found_java.push(JavaRuntime {
                version,
                executable,
            });

            debug_logger.log_verbose(format!("Found Java {} version {}",
                                             path_str, self.found_java.last().unwrap().version));
        }
    }

    fn search_in_directory(&mut self, dir: &Path, java_exe_name: &str) {
        let debug_logger = DebugLogger::global();
        debug_logger.log_verbose(format!("Searching in directory: {}", dir.display()));

        if let Ok(entries) = std::fs::read_dir(dir) {
            for entry in entries.filter_map(Result::ok) {
                if let Ok(file_type) = entry.file_type() {
                    if file_type.is_dir() {
                        let java_executable = entry.path().join("bin").join(java_exe_name);
                        self.try_add_java(java_executable);
                    }
                }
            }
        }
    }

    fn search_in_program_files(&mut self, program_files: &Path, java_exe_name: &str) {
        let vendors = ["Java", "Microsoft", "BellSoft", "Zulu",
            "Eclipse Foundation", "AdoptOpenJDK", "Semeru"];

        for vendor in vendors {
            let vendor_dir = program_files.join(vendor);
            if vendor_dir.exists() {
                self.search_in_directory(&vendor_dir, java_exe_name);
            }
        }
    }

    fn search_in_registry(&mut self, subkey: &str, java_exe_name: &str) {
        let debug_logger = DebugLogger::global();
        debug_logger.log_verbose(format!("Searching in registry: {}", subkey));

        let subkey_wide: Vec<u16> = OsString::from(subkey)
            .encode_wide()
            .chain(Some(0))
            .collect();

        let mut hkey: HKEY = HKEY::default();

        unsafe {
            if RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                PCWSTR(subkey_wide.as_ptr()),
                Some(0),
                KEY_READ | KEY_WOW64_64KEY,
                &mut hkey,
            ) != windows::Win32::Foundation::WIN32_ERROR(0) {
                return;
            }
        }

        let mut num_subkeys = 0u32;
        let mut max_subkey_len = 0u32;

        unsafe {
            if RegQueryInfoKeyW(
                hkey,
                Some(PWSTR::null()),
                None,
                None,
                Some(&mut num_subkeys),
                Some(&mut max_subkey_len),
                None,
                None,
                None,
                None,
                None,
                None,
            ) != windows::Win32::Foundation::WIN32_ERROR(0) {
                let _ = RegCloseKey(hkey);
                return;
            }
        }

        let mut buffer = vec![0u16; (max_subkey_len + 1) as usize];

        for i in 0..num_subkeys {
            let mut buffer_len = buffer.len() as u32;

            unsafe {
                if RegEnumKeyExW(
                    hkey,
                    i,
                    Some(PWSTR(buffer.as_mut_ptr())),
                    &mut buffer_len,
                    None,
                    Some(PWSTR::null()),
                    None,
                    None,
                ) != windows::Win32::Foundation::WIN32_ERROR(0) {
                    continue;
                }
            }

            let java_home_path = buffer[..buffer_len as usize]
                .iter()
                .map(|&c| c as u16)
                .collect::<Vec<u16>>();

            let mut java_home = vec![0u16; 1024];
            let mut java_home_size = (java_home.len() * 2) as u32;

            unsafe {
                if RegGetValueW(
                    hkey,
                    PCWSTR(java_home_path.as_ptr()),
                    PCWSTR("JavaHome\0".encode_utf16().collect::<Vec<_>>().as_ptr()),
                    RRF_RT_REG_SZ,
                    None,
                    Some(java_home.as_mut_ptr() as *mut _),
                    Some(&mut java_home_size),
                ) == windows::Win32::Foundation::WIN32_ERROR(0) {
                    let java_home_str = String::from_utf16_lossy(
                        &java_home[..(java_home_size as usize / 2) - 1]
                    );

                    let java_executable = Path::new(&java_home_str)
                        .join("bin")
                        .join(java_exe_name);

                    self.try_add_java(java_executable);
                }
            }
        }

        unsafe {
            let _ = RegCloseKey(hkey);
        }
    }

    pub fn best_java(&self) -> Option<&JavaRuntime> {
        self.found_java.iter().max_by_key(|r| &r.version)
    }
}

pub struct JavaLauncher {
    java_executable: PathBuf,
    workdir: PathBuf,
    jar_path: Option<PathBuf>,
    jvm_options: Option<String>,
}

impl JavaLauncher {
    pub fn new(
        java_executable: PathBuf,
        workdir: PathBuf,
        jar_path: Option<PathBuf>,
        jvm_options: Option<String>,
    ) -> Self {
        Self {
            java_executable,
            workdir,
            jar_path,
            jvm_options,
        }
    }

    pub fn launch(&self) -> std::io::Result<()> {
        use std::process::Command;

        let debug_logger = DebugLogger::global();

        let mut command = Command::new(&self.java_executable);
        command.current_dir(&self.workdir);

        if let Some(options) = &self.jvm_options {
            command.args(options.split_whitespace());
        } else {
            command.args(&["-Xmx1G", "-XX:MinHeapFreeRatio=5", "-XX:MaxHeapFreeRatio=15"]);
        }

        if let Some(jar_path) = &self.jar_path {
            command.arg("-jar").arg(jar_path);
        }

        debug_logger.log(format!("Launching Java: {:?}", command));

        let mut child = command.spawn()?;

        // 等待进程结束
        let _ = child.wait();

        Ok(())
    }
}