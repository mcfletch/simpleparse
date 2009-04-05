"""Script to automatically generate PyTable documentation"""
import pydoc2

if __name__ == "__main__":
	excludes = [
		"Numeric",
		"_tkinter",
		"Tkinter",
		"math",
		"string",
		"twisted",
	]
	stops = [
	]

	modules = [
		'simpleparse',
		'simpleparse.common',
		'simpleparse.stt',
		'simpleparse.stt.TextTools',
		'simpleparse.stt.TextTools.mxTextTools',
		'simpleparse.examples',
		'simpleparse.tests',
		'simpleparse.xmlparser',
		'__builtin__',
	]	
	pydoc2.PackageDocumentationGenerator(
		baseModules = modules,
		destinationDirectory = ".",
		exclusions = excludes,
		recursionStops = stops,
	).process ()
	
