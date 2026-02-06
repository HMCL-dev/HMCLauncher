use std::fs::{File, OpenOptions};
use std::io::{Write};
use std::sync::{Mutex, OnceLock};
use std::time::{SystemTime, UNIX_EPOCH};

pub struct DebugLogger {
    file: Mutex<Option<File>>,
    verbose: bool,
}

static GLOBAL_LOGGER: OnceLock<DebugLogger> = OnceLock::new();

impl DebugLogger {
    pub fn init() -> &'static DebugLogger {
        GLOBAL_LOGGER.get_or_init(|| {
            let verbose = std::env::var_os("HMCL_LAUNCHER_VERBOSE_OUTPUT")
                .map(|v| v != "false")
                .unwrap_or(false);

            let logger = DebugLogger {
                file: Mutex::new(None),
                verbose,
            };

            if let Ok(mut path) = std::env::current_dir() {
                path.push("logs");
                let _ = std::fs::create_dir_all(&path);

                for i in 0..9 {
                    let mut log_path = path.join("hmclauncher.log");
                    if i > 0 {
                        log_path = path.join(format!("hmclauncher.log.{}", i));
                    }

                    if let Ok(file) = OpenOptions::new()
                        .create(true)
                        .write(true)
                        .truncate(true)
                        .open(&log_path)
                    {
                        *logger.file.lock().unwrap() = Some(file);
                        break;
                    }
                }
            }

            logger
        })
    }

    pub fn global() -> &'static DebugLogger {
        GLOBAL_LOGGER.get().expect("DebugLogger not initialized")
    }

    pub fn log(&self, message: String) {
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs();

        let hours = (timestamp / 3600) % 24;
        let minutes = (timestamp / 60) % 60;
        let seconds = timestamp % 60;

        let formatted = format!("[{:02}:{:02}:{:02}] [HMCLauncher] {}",
                                hours, minutes, seconds, message);

        println!("{}", formatted);

        if let Some(ref mut file) = *self.file.lock().unwrap() {
            let _ = writeln!(file, "{}", formatted);
        }
    }

    pub fn log_verbose(&self, message: String) {
        if self.verbose {
            self.log(message);
        }
    }
}