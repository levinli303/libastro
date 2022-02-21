Pod::Spec.new do |spec|
  spec.name         = "Astronomy"
  spec.version      = "0.0.16"
  spec.summary      = "Astronomy calculation."
  spec.homepage     = "https://github.com/levinli303/libastro.git"
  spec.license      = "MIT"
  spec.author             = { "Levin Li" => "lilinfeng303@outlook.com" }

  spec.ios.deployment_target = "11.0"
  spec.osx.deployment_target = "10.9"

  spec.source       = { :git => "https://github.com/levinli303/libastro.git", :tag => "#{spec.version}" }

  spec.source_files = ["astro_common.{h,cpp}", "Astro/**/*.{h,c,mm}"]
  spec.public_header_files = ["Astronomy/Astronomy.h", "Astro/ASOAstro.h"]
end
