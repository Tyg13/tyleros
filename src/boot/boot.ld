SECTIONS {
    .boot1 0x500 : AT(0x0) { *boot1* }
    .boot2 0x700 : AT(0x0) { *boot2* }
}
