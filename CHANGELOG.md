# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- N/A

### Changed
- N/A

### Removed
- N/A

## [5.0.0] - 2017-08-21
### Added
- Model that prevents false positives in the Coverity Static Analysis Tool
- Zombie Simulator
- Logo and Software Info Badges to README
- Contributors to the README
- CHANGELOG
- Added Log Directory Command-Line Argument

### Changed
- .gitignore to ignore additional IDE artifacts
- Update Expected Values for Real Test based on Zombie Simulator
- Change Trace File Simulator Version Number to use Semantic Versioning Semantics
- Changed -l Command-Line Argument to be Only the Log File Name
- Change Log File Opening Mechanism to be more Redundant
- Changed Regression Tests for Log Directory Argument
- Updated Version Number to 5.0.0
- Updated CHANGELOG

### Removed
- RealAllocator

## [4.0.0] - 2017-08-01
### Added
- Remainder of the Region Based Garbage Collection scheme Balanced Garbage Collection.
- Arraylets; Arrays that are larger than a Region in Region Based Garbage Collection schemes.
- Additional Stats in the stdout and in the log files.
- Converted many types to size_t for compatibility.
- Test Script; See Tests/TestScripts/TestMain.sh and Tests/TestScripts/README for more details.
- "Automated" CI testing for GitLab.
- Contribution Guide; See CONTRIBUTING.md for more details.

### Changed
- Fixed some issues identified by Coverity
- Various Defect Fixes and Code Improvements.

## [3.1.0] - 2015-10-01
### Changed
- Fixed for a bug which was affecting garbage collection behavior.

## [3.0.0] - 2015-09-23
### Changed
- First Official Cut of the Trace File Simulator that marks a stable release which is compatible with version
3.0 TraceFiles.

[Unreleased]: https://github.com/GarCoSim/TraceFileSim/compare/v5.0.0...HEAD
[5.0.0]: https://github.com/GarCoSim/TraceFileSim/compare/v4.0.0...v5.0.0
[4.0.0]: https://github.com/GarCoSim/TraceFileSim/compare/v3.1...v4.0.0
[3.1.0]: https://github.com/GarCoSim/TraceFileSim/compare/v3.0...v3.1
[3.0.0]: https://github.com/GarCoSim/TraceFileSim/compare/145e471...v3.0
