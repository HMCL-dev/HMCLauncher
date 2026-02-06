use once_cell::sync::Lazy;

pub struct I18n {
    pub error_self_path: &'static str,
    pub error_invalid_hmcl_java_home: &'static str,
    pub error_java_notfound: &'static str,
}
use windows::Win32::Globalization::GetUserDefaultUILanguage;

impl I18n {
    pub fn load() -> &'static I18n {
        static I18N: Lazy<I18n> = Lazy::new(|| {
            let language = unsafe { GetUserDefaultUILanguage() };

            if language == 2052 { // zh-CN
                I18n {
                    error_self_path: "获取程序路径失败。",
                    error_invalid_hmcl_java_home: "HMCL_JAVA_HOME 所指向的 Java 路径无效，请更新或删除该变量。\n",
                    error_java_notfound: "HMCL 需要 Java 17 或更高版本才能运行，点击“确定”开始下载 Java。\n请在安装 Java 完成后重新启动 HMCL。",
                }
            } else {
                I18n {
                    error_self_path: "Failed to get the exe path.",
                    error_invalid_hmcl_java_home: "The Java path specified by HMCL_JAVA_HOME is invalid. Please update it to a valid Java installation path or remove this environment variable.",
                    error_java_notfound: "HMCL requires Java 17 or later to run,\nClick 'OK' to start downloading java.\nPlease restart HMCL after installing Java.",
                }
            }
        });

        &I18N
    }
}