// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "libastro",
    products: [
        .library(
            name: "Astronomy",
            targets: ["Astronomy"]),
    ],
    targets: [
        .target(
            name: "Astronomy",
            path: "Astro",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("ephem"),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx17
)
