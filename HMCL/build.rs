fn main() {
    let mut res = winres::WindowsResource::new();
    res.set_icon("resources/HMCL.ico")
        .set("FileVersion", "3.7.1")
        .set("ProductVersion", "3.7.1")
        .set("CompanyName", "huanghongxun")
        .set("FileDescription", "Hello Minecraft! Launcher")
        .set("LegalCopyright", "Copyright (C) 2025 huangyuhui")
        .set("ProductName", "Hello Minecraft! Launcher")
        .set("OriginalFilename", "HMCL.exe");
    res.compile().unwrap();
}
