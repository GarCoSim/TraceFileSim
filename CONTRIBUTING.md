The purpose of this document is to outline how we commit our source changes to the TraceFileSim project.

## For internal developers of the TraceFileSim (those with commit rights on GitHub):

	The guiding principle in internal development is to submit your work into the repository without breaking other people's work.
	When you commit, make sure that the repository compiles, that all the regression tests run, and that you did not clobber
	someone else's work. In the event that you are responsible for "breaking the build", fix the build at top priority.

	We have some guidelines in place to help catch most of these problems. They are as follows:

	-  Before you commit, your code MUST pass the check-in regression test. The check-in regression test is a quick way to test
	if any part of the TraceFileSim is broken. To run the check-in regression test:

		* Go to Tests/TestScripts/
		* Enter the command "./TestMain.sh SyntheticBaseCasesInput.txt -all"
		* Re-Run TestMain.sh substitutung "SyntheticInput, RealInput, etc." for "SyntheticBaseCasesInput" and specifying the "-all" flag.
		* You may commit if ALL the regression tests complete with no errors.

	-  Everyone who is doing development must write a regression test for each major feature that they've created. This is
	important because if someone (or yourself) breaks some new feature, the regression tests will catch it. (Instructions or Writing
	and modifying tests can be found in Tests/TestScripts/README)

	-  In the event a regression test is broken, the one responsible for having the test pass is in charge of determining: a) If there
	is a defect in the source code, in which case the source code needs to be updated to fix the defect or b) If there is a problem
	with the test (perhaps the quality of the tool did in fact get better or perhaps there is a defect with the test itself), in which case
	the test needs to be updated to reflect the new changes.

	-  Update regularly and commit as regularly as you can. The longer code deviates from the HEAD of the master branch, the
	more painful it is to integrate back into master.

	Whatever system that we come up with will not be foolproof so be conscientious about team development.

## For external developers of TraceFileSim:

	If you are working on the TraceFileSim - Merge with the latest revision of the TraceFileSim. Create a Pull Request or submit
	your patch/source code to ken@unb.ca with a description of what the changes entail.
