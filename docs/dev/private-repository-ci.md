# Using private repositories in CI

This document uses the following naming convention throughout:
- **Working Repository** is the GitHub repo that you are working on. It needs access to code from another repository, called the...
- **External Repository** is the repository you are fetching from CMake, which is not publicly available and therefore needs an access token.

When locally building a project containing a CMake file which fetches dependencies to other private repositories (e.g. building the Environment Sensor requires dependencies from cascoda-sdk-priv), this will typically be no issue because your username/email/password credentials on github will be used to give you permission to fetch those dependencies. However, CI isn't capable of doing the same thing, and this will result in a failed build due to the inability to fetch the dependencies. There needs to be some mechanism to allow a CI build to authenticate itself to the private repository from which the dependencies need to be fetched. This is what this guide will address.

This guide describes how to enable a CMake script in your Working Repository to have access to dependencies available in another External Repository. Note: You will only be required to modify the CMake & GitHub Actions scripts in the Working Repository. You do NOT need to make any changes to codegen or to the External Repository from which you need the dependencies.

## The Problem

You are working on a project which needs to clone an external repository. Normally, you would write the following CMake:

```cmake
FetchContent_Declare(
	cascoda-sdk
    GIT_REPOSITORY https://github.com/Cascoda/cascoda-sdk-priv.git
    GIT_TAG master
)

FetchContent_MakeAvailable(cascoda-sdk)
```

This works locally, but not when running in CI.

When CMake executes `FetchContent_MakeAvailable`, it performs a `git clone` of the repository described by `FetchContent_Declare`, and if it is private, it prompts the
user for a password on the command line interface. This last step fails in CI,
as seen in this example build log:
```
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Fetching KNX-IoT Source Code, please wait...
[ 11%] Creating directories for 'cascoda-sdk-populate'
-- Configuring incomplete, errors occurred!
[ 22%] Performing download step (git clone) for 'cascoda-sdk-populate'
Cloning into 'cascoda-sdk-src'...
fatal: could not read Username for 'https://github.com': No such device or address
Cloning into 'cascoda-sdk-src'...
fatal: could not read Username for 'https://github.com': No such device or address
Cloning into 'cascoda-sdk-src'...
fatal: could not read Username for 'https://github.com': No such device or address
-- Had to git clone more than once: 3 times.
CMake Error at cascoda-sdk-subbuild/cascoda-sdk-populate-prefix/tmp/cascoda-sdk-populate-gitclone.cmake:39 (message):
  Failed to clone repository: 'https://github.com/cascoda/cascoda-sdk-priv.git'
```

By default, a Github Action only has access to the repository it runs on, and it is not aware of any username or password it could use for a `git clone` prompt. Login credentials need to be injected for the clone to succeed.

## The Solution

### Step 1a: Identify if there is already an Organization Secret that you can use

Organization secrets are usable by Cascoda developers, within repositories owned by the Cascoda organization.

If the External Repository is the private Cascoda SDK, you can use `CASCODA_PRIVATE`.

If the External Repository is Cascoda's fork of the KNX-IoT Stack, you can use `KNX_IOT_STACK_SECRET`

If you are using one of these secrets, you may skip Step 1b and Step 2.

### Step 1b: Create a Personal Access Token

A Personal Access Token is an alternative to using passwords for authentication to GitHub. Fine-grained access tokens should be used whenever possible, containing only the necessary permissions.

Note that fine-grained tokens expire after some time. When the token expires & CI starts failing to clone repositories, you will have to generate a new one and replace it within the repository's secrets. Fine-grained access tokens are also revoked when the user who generated them loses access to the resource, such as by leaving the organisation.

Navigate to the [Fine-grained personal access token configuration page.](https://github.com/settings/tokens?type=beta) Generate a new token, setting the Resource owner to Cascoda. Under Repository access, pick "Only select repositories", and then select the External Repository you would like to access from the CI of your Working Repository. Finally, you must allow Read-only access to Contents, under the Permissions section.

Once you have generated the access token, copy it to your clipboard and proceed to the next section.

### Step 2: Create a new secret in your Working Repository's GitHub remote

Open your Working Repository's GitHub page, and navigate to Settings -> Secrets and Variables -> Actions. Create a name for your secret (e.g. PROJECT_SECRET), and paste the secret in the box below.

**Note for Codegen Action users:** if you pick the name "THIRD_PARTY_TOKEN", it will automatically be included within the action's environment, and you may skip Step 3.

### Step 3: Add the secret to the Working Repository action environment

At the top of the workflow file (stored in `.github/workflows`), set an environment variable containing the secret. This will make it usable by the entire workflow. For example:
```yaml
name: Linux

# Add secret to environment
env:
  PROJECT_SECRET: ${{secrets.PROJECT_SECRET}}

on:
  push:
    branches: [ master ]
  pull_request:
...
```

### Step 4: Use the secret environment variable within CMake

In the Working Repository, modify the CMake to use the secret environment variable to login into GitHub, by changing the value of the GIT_REPOSITORY argument. 

```cmake
FetchContent_Declare(
	cascoda-sdk
    GIT_REPOSITORY https://$ENV{PROJECT_SECRET}@github.com/Cascoda/cascoda-sdk-priv.git
    GIT_TAG master
)

FetchContent_MakeAvailable(cascoda-sdk)
```

Note that you can still build locally: if the secret is not present, the `$ENV{}` evaluates to an empty string and you will be prompted to type in your password instead.
