#![windows_subsystem = "windows"]
mod debug;
mod i18n;
mod java;

use windows::{
    Win32::UI::WindowsAndMessaging::{
        MessageBoxW, IDOK, MB_ICONERROR, MB_ICONWARNING, MB_OK, MB_OKCANCEL,
    },
    Win32::System::Console::{AttachConsole, ATTACH_PARENT_PROCESS},
};

use std::env;
use std::process;
use std::ffi::OsString;
use anyhow::{Context, Result};
use std::path::{Path, PathBuf};

use debug::DebugLogger;
use i18n::I18n;
use java::{JavaFinder, JavaLauncher};
use java::platform::{Architecture, Platform};

fn main() -> Result<()> {
    let debug_logger = DebugLogger::init();

    // Check if it needs to show detailed information
    env::var_os("HMCL_LAUNCHER_VERBOSE_OUTPUT")
        .map(|v| v != "false")
        .unwrap_or(false);

    let _ = unsafe { AttachConsole(ATTACH_PARENT_PROCESS) };

    let i18n = I18n::load();
    let platform = Platform::current();

    debug_logger.log(format!("*** HMCL Launcher {} ***", env!("CARGO_PKG_VERSION")));
    debug_logger.log(format!("System Architecture: {:?}", platform.architecture));

    let self_exe_path = env::current_exe()
        .context(i18n.error_self_path)?;
    debug_logger.log(format!("Working directory: {}", self_exe_path.parent().unwrap_or(&self_exe_path).display()));
    debug_logger.log(format!("Exe File: {}", self_exe_path.display()));

    let java_exe_name = if unsafe { AttachConsole(ATTACH_PARENT_PROCESS) }.is_ok() {
        "java.exe"
    } else {
        "javaw.exe"
    };

    // If HMCL_JAVA_HOME is set, it should always be used
    if let Some(hmcl_java_home) = env::var_os("HMCL_JAVA_HOME") {
        debug_logger.log(format!("HMCL_JAVA_HOME: {}", hmcl_java_home.to_string_lossy()));

        let java_executable = Path::new(&hmcl_java_home).join("bin").join(java_exe_name);
        if java_executable.is_file() {
            let launcher = JavaLauncher::new(
                java_executable.clone(),
                self_exe_path.parent().unwrap_or(&self_exe_path).to_path_buf(),
                self_exe_path.file_name().map(|s| PathBuf::from(s)),
                env::var("HMCL_JAVA_OPTS").ok(),
            );

            if launcher.launch().is_ok() {
                return Ok(());
            }
        }

        // HMCL_JAVA_HOME is set but not valid
        show_message_box(
            &i18n.error_invalid_hmcl_java_home,
            None,
            (MB_ICONERROR | MB_OK).0,
        );
        process::exit(1);
    }

    if let Err(e) = find_and_launch_java(&platform, &self_exe_path, java_exe_name, &i18n) {
        debug_logger.log(format!("Failed to launch Java: {}", e));

        let download_link = match platform.architecture {
            Architecture::Arm64 => "https://docs.hmcl.net/downloads/windows/arm64.html",
            Architecture::X86_64 => "https://docs.hmcl.net/downloads/windows/x86_64.html",
            Architecture::X86 => "https://docs.hmcl.net/downloads/windows/x86.html",
        };

        if show_message_box(
            &i18n.error_java_notfound,
            None,
            (MB_ICONWARNING | MB_OKCANCEL).0,
        ) == IDOK.0 {
            let _ = open_url(download_link);
        }

        process::exit(1);
    }

    Ok(())
}

fn find_and_launch_java(
    platform: &Platform,
    self_exe_path: &Path,
    java_exe_name: &str,
    _i18n: &I18n,
) -> Result<()> {
    const EXPECTED_JAVA_MAJOR_VERSION: u16 = 17;

    let debug_logger = DebugLogger::global();

    let mut finder = JavaFinder::new(EXPECTED_JAVA_MAJOR_VERSION);

    finder.find_java_installations(java_exe_name);

    let workdir = self_exe_path.parent().unwrap_or(self_exe_path);
    let bundled_jre_path = get_bundled_jre_path(platform, workdir, java_exe_name);

    if bundled_jre_path.is_file() {
        debug_logger.log(format!("Bundled JRE: {}", bundled_jre_path.display()));

        let launcher = JavaLauncher::new(
            bundled_jre_path,
            workdir.to_path_buf(),
            self_exe_path.file_name().map(|s| PathBuf::from(s)),
            env::var("HMCL_JAVA_OPTS").ok(),
        );

        return Ok(launcher.launch()?);
    }

    debug_logger.log("Bundled JRE: Not Found".to_string());


    if let Some(best_java) = finder.best_java() {
        debug_logger.log(format!("Using Java: {} (Version {})",
                                 best_java.executable.display(),
                                 best_java.version));

        let launcher = JavaLauncher::new(
            best_java.executable.clone(),
            workdir.to_path_buf(),
            self_exe_path.file_name().map(|s| PathBuf::from(s)),
            env::var("HMCL_JAVA_OPTS").ok(),
        );

        return Ok(launcher.launch()?);
    }

    anyhow::bail!("No suitable Java installation found");
}

fn get_bundled_jre_path(platform: &Platform, workdir: &Path, java_exe_name: &str) -> PathBuf {
    let jre_dir = match platform.architecture {
        Architecture::Arm64 => "jre-arm64",
        Architecture::X86_64 => "jre-x64",
        Architecture::X86 => "jre-x86",
    };

    workdir.join(jre_dir).join("bin").join(java_exe_name)
}

fn show_message_box(text: &str, caption: Option<&str>, flags: u32) -> i32 {
    use std::os::windows::ffi::OsStrExt;
    use windows::core::PCWSTR;

    let text_wide: Vec<u16> = OsString::from(text).encode_wide().chain(Some(0)).collect();
    let caption_wide: Vec<u16> = caption
        .map(|c| OsString::from(c).encode_wide().chain(Some(0)).collect())
        .unwrap_or_else(|| vec![0]);

    unsafe {
        MessageBoxW(
            None,
            PCWSTR(text_wide.as_ptr()),
            PCWSTR(caption_wide.as_ptr()),
            windows::Win32::UI::WindowsAndMessaging::MESSAGEBOX_STYLE(flags),
        ).0 as i32
    }
}

fn open_url(url: &str) -> Result<()> {
    use windows::Win32::UI::Shell::ShellExecuteW;
    use windows::core::PCWSTR;
    use std::os::windows::ffi::OsStrExt;

    let url_wide: Vec<u16> = OsString::from(url).encode_wide().chain(Some(0)).collect();
    let operation_wide: Vec<u16> = OsString::from("open").encode_wide().chain(Some(0)).collect();

    unsafe {
        ShellExecuteW(
            None,
            PCWSTR(operation_wide.as_ptr()),
            PCWSTR(url_wide.as_ptr()),
            None,
            None,
            windows::Win32::UI::WindowsAndMessaging::SHOW_WINDOW_CMD(5), // SW_SHOW
        );
    }

    Ok(())
}