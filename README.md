# Gherkin Executor for C++

The Gherkin Executor for C++ converts Gherkin files into unit tests.   It is currently underdevelopment, but its basic operations work.   

See the full documentation at [GitHub - atdd-bdd/GherkinExecutorBase: This is the base for Gherkin Executor containing Documentation and Examples](https://github.com/atdd-bdd/GherkinExecutorBase)

You can see an example of a featurex file at:

https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/blob/main/examples.featurex

The generated code, as well as the altered glue file are in this directory:: 

https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/tree/main/gherkinexecutor/Feature_Examples

This documents the setup required for C++.    It assumes you have some experience with running unit tests in Visual Studio.   

### Setup

- Clone https://github.com/atdd-bdd/GherkinExecutorCPP and compile it.    There will be a standalone .exe file, once the  

- Create a gtest test project.   Other unit test frameworks will be ready shortly.    

- Go to https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP

- Download` configuration.yaml `  [https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/blob/main/Configuration.yaml](https://github.com/atdd-bdd/GherkinExecutorForCSharp/blob/main/Translaste.cs)
  Download` starting.featurex` https://github.com/atdd-bdd/TestsForGherkinExecutorForCPP/blob/main/starting.featurex](https://github.com/atdd-bdd/GherkinExecutorForCSharp/blob/main/GherkinExecutor/starting.featurex)

- Edit the` configuration.yaml file` and change  ` inTest: false `

- Now open up a command window and run the following,  substituting the path to the executable.  This should be run in the main directory of the test project (where the configuration and featurex files are located).   

```
{path}GherkinExecutorCPP.exe starting.featurex 
```

- A folder `gherkinexecutor/Feature_starting`is created. 

- Go to that folder.  Do two renames.   `Feature_Starting_glue.cpp.tmpl `to `Feature_Start_glue.cpp`   and   `Feature_Starting_glue.h.tmpl` to `Feature_Starting_glue. h`

- You will need to add all the files in the folder to Visual Studio (if that's what you are using).  

- Run all tests.    

- Now implement the production code to make the tests pass 

- ```
  Now implement the production code to make the test pass.
  ```

If you add a `Scenario `to the feature file, you need to rerun `GherkinExecutorForCPP`.     If you add new steps, you need to copy the new glue code from the `glue tmpl `file to the` glue cpp` file and copy the prototype from` glue.h.tmpl` to` glue.h` file.


