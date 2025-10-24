# Gherkin Executor for C++

The Gherkin Executor for C++ converts Gherkin extended files into unit tests. See the full documentation at [GitHub - atdd-bdd/GherkinExecutorBase: This is the base for Gherkin Executor containing Documentation and Examples](https://github.com/atdd-bdd/GherkinExecutorBase)

Currently `gtest `is supported.  Other frameworks will be added shortly.   The development IDE was Visual Studio, but the program should run in any C++ environment.  



You can see an example of a featurex file at:

https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/blob/main/examples.featurex



The generated code, as well as the modified glue file are in this directory:: 

https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/tree/main/gherkinexecutor/Feature_Examples



The directory contains the unit test files, the glue files,  and the data files.   The unit tests files hold the step tables converted into C++.   The glue files have a method for each step, which the developer alters to call the production code.   The data files contain the data elements converted into classes with auxillary methods.  



This documents the setup required for C++.    It assumes you have some experience with running unit tests in Visual Studio.   

### Setup

- Clone https://github.com/atdd-bdd/GherkinExecutorCPP and compile it.    There will be a standalone .exe file.   There is an exe file for Windows X64 provide.    

- Create a `gtest `test project.   Other unit test frameworks will be ready shortly.    

- Go to https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP

- Download` configuration.yaml `  [https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/blob/main/Configuration.yaml](https://github.com/atdd-bdd/GherkinExecutorForCSharp/blob/main/Translaste.cs)
  Download` starting.featurex` https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/blob/main/starting.featurex](https://github.com/atdd-bdd/GherkinExecutorForCSharp/blob/main/GherkinExecutor/starting.featurex)

- Now open up a command window and run the following,  substituting the path to the executable.  This should be run in the main directory of the test project (where the configuration and featurex files are located).   

```
{path}GherkinExecutorCPP.exe starting.featurex 
```

- A folder `gherkinexecutor/Feature_starting`is created. 

- Go to that folder.  Do two renames.   `Feature_Starting_glue.cpp.tmpl `to `Feature_Start_glue.cpp`   and   `Feature_Starting_glue.h.tmpl` to `Feature_Starting_glue. h`

- You will need to add all the files in the folder to Visual Studio (if that's what you are using).  

- Run all tests.    

- Now implement the production code to make the tests pass 
  
  

If you add a `Scenario `to the feature file, you need to rerun `GherkinExecutorForCPP`.     If you add new steps, you need to copy the new glue code from the `glue.cpp.tmpl `file to the` glue.cpp` file and copy the prototype from` glue.h.tmpl` to` glue.h` file.   Alternatively, you can correct the missing method error in the test file by having the IDE create the method.    
