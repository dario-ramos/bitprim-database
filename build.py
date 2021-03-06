from conan.packager import ConanMultiPackager

if __name__ == "__main__":
    builder = ConanMultiPackager(username="bitprim", channel="stable",
                                 remotes="https://api.bintray.com/conan/bitprim/bitprim",
                                 archs=["x86_64"])

    builder.add_common_builds(shared_option_name="bitprim-database:shared")

    filtered_builds = []
    for settings, options, env_vars, build_requires in builder.builds:
        if settings["build_type"] == "Release" \
                and not options["bitprim-database:shared"] \
                and ("compiler.runtime" not in settings or not settings["compiler.runtime"] == "MT"):
            filtered_builds.append([settings, options, env_vars, build_requires])

    builder.builds = filtered_builds
    builder.run()
