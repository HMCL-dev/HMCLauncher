use std::fmt;
use std::os::windows::ffi::OsStrExt;
use windows::core::w;
use windows::Win32::Storage::FileSystem::VerQueryValueW;
use windows::Win32::Storage::FileSystem::VS_FIXEDFILEINFO;
use windows::Win32::Storage::FileSystem::GetFileVersionInfoW;
use windows::Win32::Storage::FileSystem::GetFileVersionInfoSizeW;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct JavaVersion {
    pub major: u16,
    pub minor: u16,
    pub build: u16,
    pub revision: u16,
}

impl JavaVersion {
    pub fn from_java_executable(path: &std::path::Path) -> Option<Self> {
        use windows::core::PCWSTR;

        let path_wide: Vec<u16> = path
            .as_os_str()
            .encode_wide()
            .chain(Some(0))
            .collect();

        let size = unsafe {
            GetFileVersionInfoSizeW(PCWSTR(path_wide.as_ptr()), None)
        };

        if size == 0 {
            return None;
        }

        let mut buffer = vec![0u8; size as usize];

        unsafe {
            if GetFileVersionInfoW(
                PCWSTR(path_wide.as_ptr()),
                Some(0),
                size,
                buffer.as_mut_ptr() as _,
            ).is_err() {
                return None;
            }
        }

        let mut file_info: *const VS_FIXEDFILEINFO = std::ptr::null();
        let mut file_info_size = 0u32;

        unsafe {
            if !VerQueryValueW(
                buffer.as_ptr() as _,
                PCWSTR::from_raw(w!("\\\0").as_ptr()),
                &mut file_info as *mut _ as _,
                &mut file_info_size,
            ).as_bool() {
                return None;
            }
        }

        if file_info_size == 0 || file_info.is_null() {
            return None;
        }

        let file_info = unsafe { *file_info };

        Some(JavaVersion {
            major: ((file_info.dwFileVersionMS >> 16) & 0xFFFF) as u16,
            minor: (file_info.dwFileVersionMS & 0xFFFF) as u16,
            build: ((file_info.dwFileVersionLS >> 16) & 0xFFFF) as u16,
            revision: (file_info.dwFileVersionLS & 0xFFFF) as u16,
        })
    }
    pub fn is_acceptable(&self, expected_major: u16) -> bool {
        self.major >= expected_major
    }
}

impl fmt::Display for JavaVersion {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.major == 0 {
            write!(f, "Unknown")
        } else {
            write!(f, "{}.{}.{}.{}", self.major, self.minor, self.build, self.revision)
        }
    }
}