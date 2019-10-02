SECTIONS {
    .boot1 0x0500 : AT(0x000) { *boot1* }
    .boot2 0x7C00 : AT(0x200) { *boot2* }
}
