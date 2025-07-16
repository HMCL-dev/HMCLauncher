/*
 * Hello Minecraft! Launcher
 * Copyright (C) 2025 huangyuhui <huanghongxun2008@126.com> and contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import java.security.MessageDigest

plugins {
    id("java-library")
    id("signing")
    id("maven-publish")
    id("io.github.gradle-nexus.publish-plugin") version "2.0.0"
    id("de.undercouch.download") version "5.6.0"
    id("org.glavo.load-maven-publish-properties") version "0.1.0"
}

group = "org.glavo.hmcl"
version = project.findProperty("version") ?: throw GradleException("Version not specified")
description = "HMCLauncher for Windows"

val downloadDir = layout.buildDirectory.dir("downloads")

val downloadVerifyFile by tasks.registering(de.undercouch.gradle.tasks.download.Download::class) {
    src("https://github.com/HMCL-dev/HMCLauncher/releases/download/$version/HMCLauncher.exe.sha256")
    dest(downloadDir.map { it.file("HMCLauncher-$version.exe.sha256") })
    overwrite(false)
}

val downloadLauncher by tasks.registering(de.undercouch.gradle.tasks.download.Download::class) {
    src("https://github.com/HMCL-dev/HMCLauncher/releases/download/$version/HMCLauncher.exe")
    dest(downloadDir.map { it.file("HMCLauncher-$version.exe") })
    overwrite(false)
}

val verifyLauncher by tasks.registering {
    dependsOn(downloadLauncher, downloadVerifyFile)

    doLast {
        val expectedChecksum = downloadVerifyFile.get().outputFiles.single().readText().trim()
        val actualChecksum = downloadLauncher.get().outputFiles.single().readBytes().let { bytes ->
            MessageDigest.getInstance("SHA-256").digest(bytes).joinToString("") { "%02x".format(it) }
        }

        if (!expectedChecksum.equals(actualChecksum, true)) {
            throw GradleException("Checksum verification failed: expected $expectedChecksum, got $actualChecksum")
        }
    }
}

tasks.processResources {
    dependsOn(downloadLauncher, verifyLauncher)

    into("assets") {
        from(downloadLauncher.map { it.outputFiles.single() }) {
            rename { "HMCLauncher.exe" }
        }
    }
}

java {
    withJavadocJar()
    withSourcesJar()
}

publishing {
    publications {
        create<MavenPublication>("hmclauncher") {
            from(components["java"])

            pom {
                name.set(project.name)
                description.set(project.description)
                url.set("https://github.com/HMCL-dev/HMCLauncher")
                licenses {
                    license {
                        name.set("GPL 3.0")
                        url.set("https://www.gnu.org/licenses/gpl-3.0.html")
                    }
                }
                developers {
                    developer {
                        id.set("huanghongxun")
                        name.set("Yuhui Huang")
                        email.set("jackhuang1998@gmail.com")
                    }

                    developer {
                        id.set("Glavo")
                        name.set("Glavo")
                        email.set("zjx001202@gmail.com")
                    }
                }
                scm {
                    url.set("https://github.com/HMCL-dev/HMCLauncher")
                }
            }
        }
    }
}

signing {
    useInMemoryPgpKeys(
        rootProject.ext["signing.keyId"].toString(),
        rootProject.ext["signing.key"].toString(),
        rootProject.ext["signing.password"].toString(),
    )
    sign(publishing.publications["hmclauncher"])
}

// ./gradlew publishToSonatype closeAndReleaseSonatypeStagingRepository
nexusPublishing {
    repositories {
        sonatype {
            nexusUrl.set(uri("https://ossrh-staging-api.central.sonatype.com/service/local/"))
            snapshotRepositoryUrl.set(uri("https://central.sonatype.com/repository/maven-snapshots/"))

            username.set(rootProject.ext["sonatypeUsername"].toString())
            password.set(rootProject.ext["sonatypePassword"].toString())
        }
    }
}