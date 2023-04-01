### How to Contribute

We'd love to accept your patches and contributions to this project. There are just a few small guidelines you need to follow.

## Create a pull request

Please follow [making a pull request](https://docs.github.com/en/get-started/quickstart/contributing-to-projects#making-a-pull-request) guide.

Note that the pr to master branch must have a feature_ [Date] [English abbreviation for function] or bugfix [Date]_ [Functional Abbreviations] Reference Examples

examples:
```
feature_20230323_initciflow
bugfix_20230323_fixciflow
```

After submitting a PR, at least one person must approve the review before the code can be merged


## Code Style
Strictly comply with the Valley C++style specification, pay attention to specification checking during cr, or install plug-ins for specification checking in the ide.

[https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/contents/](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/contents/)

Execute the following command under the project directory to check the specifications

```
find src -type f \( -name '*.h' -or -name '*.hpp' -or -name '*.cpp' -or -name '*.c' -or -name '*.cc' \) -print | xargs clang-format -style=file --sort-includes -i -n -Werror
```

Auto Format Code
```
find src -type f \( -name '*.h' -or -name '*.hpp' -or -name '*.cpp' -or -name '*.c' -or -name '*.cc' \) -print | xargs clang-format -style=file --sort-includes -i
```

## Last

We are very open and welcome to submit code to harass us~
