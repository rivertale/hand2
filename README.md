# hand2

hand2 helps you manage your course with `GitHub` and `Google Sheets`. It incorporates features from *[invisible-hand](https://github.com/ianchen-tw/invisible-hand)* and other helper programs, such as *HW-manager*, *get-commits*.


## Table of contents
- [Install](#install)
- [Getting started](#getting-started)
    - [Create configuration](#create-configuration)
    - [Configuration](#configuration)
        - [github_access_token](#github_access_token)
        - [google_api_key](#google_api_key)
        - [organization](#organization)
        - [ta_team](#ta_team)
        - [student_team](#student_team)
        - [feedback_repository](#feedback_repository)
        - [spreadsheet](#spreadsheet)
        - [username_label](#username_label)
        - [student_id_label](#student_id_label)
        - [email](#email)
        - [grade_command](#grade_command)
        - [grade_thread](#grade_thread)
        - [score_relative_path](#score_relative_path)
        - [penalty_per_day](#penalty_per_day)
- [Workflow](#workflow)
    - [Config check](#config-check)
    - [Invite students](#invite-students)
    - [Announce homework](#announce-homework)
    - [Grade homework](#grade-homework)
    - [Clean](#clean)
- [Explanation](#explanation)
    - [Course structure](#course-structure)
    - [Timezone](#timezone)
    - [Deadline and Cutoff](#deadline-and-cutoff)
    - [Feedback format](#feedback-format)
    - [Logging](#logging)
    - [Using WSL](#using-wsl)
- [How to build](#how-to-build)
    - [Windows](#windows)
    - [Linux](#linux)
    - [Docker](#docker)
- [How to build the dependency](#how-to-build-the-dependency)
    - [Windows](#windows-1)
    - [Linux](#linux-1)
- [How to publish](#how-to-publish)
    - [Windows](#windows-2)
    - [Linux](#linux-2)
    - [Docker](#docker-1)


## Install
Download pre-built x64 binaries from [release](https://github.com/Compiler-s24/hand2/releases). You can also build from the source following instructions in [How to build](#how-to-build).


## Getting started
hand2 runs on Windows, Linux, or Docker environments. To show usage, run the following command:
> Use Docker environment is recommended.
- Windows: `.\hand.exe --help`
- Linux: `./hand --help`
- Docker: `docker/run.sh --help`


### Create configuration
If there's no existing `config.txt`, run the program with any command will create a default configuration under the executable's directory. For example:
```
./hand config-check
```
<details>
<summary>Expected output</summary>

```
Config not found, default config created at '/hand/config.txt'

usage: hand2 [--options] ... command [--command-options] ... [args] ...
[options]
    --help    show this message
    --log     log to log/YYYY-MM-DD_hh-mm-ss.log
[command]
    config-check       check if the config is valid
    invite-students    invite student into github organization
    collect-homework   collect homework and retrieve late submission info
    grade-homework     grade homework
    announce-grade     announce grade
    clean              delete unrelated files such as logs and caches
[common-command-options]
    --help             show the help message regards to the command
```

</details>


### Configuration
Before looking into the configuration, you might want to review the [course structure](#course-structure). To give you a rough idea, here's a sample `config.txt` for Compiler-s24. You can read descriptions for each field below.
> The personal information is obscured in this section.
```ini
# config.txt

github_access_token = "ghp_4TB9AKgJ2hrNKXahHbjVgE5ta1OcmgGgGzpT"
google_api_key = "AIzaSyDPyovtvneNfxbzFyD35RCjxhuvx9mZHj0"

organization = "Compiler-s24"
ta_team = "TA"
student_team = "Students"
feedback_repository = "hw-feedback"

spreadsheet = "1bB5YCp5vxkfCwlSod1z3qlsQwXZ8j9S8ijuaEptHU3w"
username_label = "github_handle"
student_id_label = "student_id"

email = "TA@gmail.com"
grade_command = 'make activate ARGS="make autograde"'
grade_thread = "8"
score_relative_path = "./test/result/score.txt"
penalty_per_day = "15"
```

<a id="github_access_token"></a>
- `github_access_token`: See how to create a GitHub classic personal access token [here](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens#creating-a-personal-access-token-classic), or follow the guide:
    <details>
    <summary>Guide</summary>

    1. Go to **Settings**.
       <br>![github-access-token-01](docs/github-access-token-01.png)
    2. Go to **Developer settings**.
       <br>![github-access-token-02](docs/github-access-token-02.png)
    3. Go to **Personal access tokens** -> **Tokens (classic)** -> **Generate new token** -> **Generate new token (classic)**.
       <br>![github-access-token-03](docs/github-access-token-03.png)
    4. Fill in **Note**, **Expiration**, and enable all **Scopes** (or you can test what scopes are needed).
       <br>![github-access-token-04](docs/github-access-token-04.png)
    5. **Generate token**.
       <br>![github-access-token-05](docs/github-access-token-05.png)
    6. Copy the access token to `config.txt`.
       <br>![github-access-token-06](docs/github-access-token-06.png)

    </details>

<a id="google_api_key"></a>
- `google_api_key`: See how to create a Google API key [here](https://support.google.com/googleapi/answer/6158862), or follow the guide:
    <details>
    <summary>Guide</summary>

    1. Go to **[API Console](https://console.cloud.google.com/apis/dashboard)** -> **Select a project** -> **New project**.
       <br>![google-api-key-01](docs/google-api-key-01.png)
    2. Fill in **Project name** -> **Create**.
       <br>![google-api-key-02](docs/google-api-key-02.png)
    3. Go to **Credentials** -> **Create Credentials** -> **API key**.
       <br>![google-api-key-03](docs/google-api-key-03.png)
    4. Go to **Edit API key**.
       <br>![google-api-key-04](docs/google-api-key-04.png)
    5. Fill in **Name** -> **Save**, and copy the API key to `config.txt`.
       <br>![google-api-key-05](docs/google-api-key-05.png)

    </details>

- We obtain the following fields from the course organization:
<a id="organization"></a>
<a id="ta_team"></a>
<a id="student_team"></a>
<a id="feedback_repository"></a>
    - `organization`
    - `ta_team`
    - `student_team`
    - `feedback_repository`
  ```
  <organization>: Compiler-s24
  │
  ├─ repository
  │  │
  │  ├─ <feedback-repository>: hw-feedback
  │  ├─ tmpl-hw1
  │  ├─ tmpl-hw2
  │  └─ ...
  │
  └─ team
     ├─ <ta-team>: TA
     └─ <student-team>: Students
  ```

<a id="spreadsheet"></a>
<a id="username_label"></a>
<a id="student_id_label"></a>
- We obtain the following fields from the spreadsheet:
    - `spreadsheet`
    - `username_label`
    - `student_id_label`
      <br>![google-spreadsheet-02](docs/google-spreadsheet-02.png)

<a id="email"></a>
- `email`: Ensure this email is linked to your GitHub account. We use it to commit feedbacks to [`feedback_repository`](#feedback_repository) when grading homework.

<a id="grade_command"></a>
- `grade_command`: Command executed under the homework directory for grading.
    - (*Note: The field value can be enclosed by single quotes(`'`) or double quotes (`"`), we use single quotes here to escape the double quotes*)

<a id="grade_thread"></a>
- `grade_thread`: Number of threads used for parallel grading.

<a id="score_relative_path"></a>
- `score_relative_path`: Path to the score file relative to the homework directory. The score file is generated by `grade_command`.

<a id="penalty_per_day"></a>
- `penalty_per_day`: Penalty received per day for late submission (in percentage).


After configuring, validate it via [config-check](#config-check).


## Workflow

### Config check
Validate the content of `config.txt`.

1. **Fill out `config.txt`.**

2. **Run command: `./hand config-check`**
   <details>
   <summary>Output</summary>

   ```

   [GitHub]
       Token owner: octocat ... OK
       Organization: Compiler-s24 ... OK
       Teaching team: TA ... OK
       Student team: Students ... OK
       Feedback repository: hw-feedback ... OK
       Commit email: TA@gmail.com ... nocheck

   [Google Sheet]
       Token ... OK
       Spreadsheet: 1bB5YCp5vxkfCwlSod1z3qlsQwXZ8j9S8ijuaEptHU3w ... OK

   [Homework sheet]
       表單回應 1: github_handle=-1, student_id=-1 ... Not a homework
       StudentInfo: github_handle=6, student_id=4 ... OK
       hw1: github_handle=9, student_id=2 ... OK
       hw2: github_handle=9, student_id=2 ... OK
       hw3: github_handle=9, student_id=2 ... OK
       hw4: github_handle=9, student_id=2 ... OK
       hw5: github_handle=9, student_id=2 ... OK
       期中考: github_handle=-1, student_id=2 ... Not a homework
       期末考: github_handle=-1, student_id=2 ... Not a homework
       Total: github_handle=-1, student_id=2 ... Not a homework
       學期成績: github_handle=-1, student_id=-1 ... Not a homework

   [Misc]
       Use 8 grading thread ... OK
       Penalty per day is 15% ... OK
       Score read from: {hwdir}/./test/result/score.txt ... nocheck
       Grading command: make activate ARGS="make autograde" ... nocheck

   [Summary]
       Config is OK
   ```

   </details>


### Invite students
Invite students to join the GitHub organization when the semester starts.

1. **Create a Google form and have students fill out the information.**

2. **Copy students' GitHub usernames into a file, each username is separated by whitespaces or newlines.** For example, We create a file `students.txt` containing:
    - (*Note: A username is not the same as a public profile name, a valid username can link to your personal page via `https://github.com/<username>`*)
   ```
   username-01
   username-02
   username-03
   username-04
   ```

3. **Run command: `./hand invite-students students.txt`**
   <details>
   <summary>Output</summary>

   ```
   Retrieving existing members...
   Retrieving existing invitations...
   Checking for valid usernames...
   Inviting new members...
   Invite 'username-01' to 'Compiler-s24/Students'
   Invite 'username-02' to 'Compiler-s24/Students'
   Invite 'username-03' to 'Compiler-s24/Students'
   Invite 'username-04' to 'Compiler-s24/Students'

   [Summary]
       Total students: 4
       New invitations (success): 4
       New invitations (failure): 0
       Existing members: 0
       Existing invitations: 0
   ```

   </details>

4. (Optional) **Update `students.txt` based on the newly filled form.** For example:
   ```
   username-01
   username-02
   username-03
   username-04
   username-05
   username-06
   username-07
   username-08
   ```

5. (Optional) **Run command again: `./hand invite-students students.txt`**
   <details>
   <summary>Output</summary>

   ```
   Retrieving existing members...
   Retrieving existing invitations...
   Checking for valid usernames...
   Inviting new members...
   Invite 'username-05' to 'Compiler-s24/Students'
   Invite 'username-06' to 'Compiler-s24/Students'
   Invite 'username-07' to 'Compiler-s24/Students'
   Invite 'username-08' to 'Compiler-s24/Students'

   [Summary]
       Total students: 8
       New invitations (success): 4
       New invitations (failure): 0
       Existing members: 0
       Existing invitations: 4
   ```

   </details>


### Announce homework
We use GitHub Classroom to distribute homework. Take `hw0` in Compiler-s24 as an example.

1. Go to **Create an assignment**.
   <br>![github-classroom-01](docs/github-classroom-01.png)

2. Fill in the **Assignment title** -> **Continue**.
   <br>![github-classroom-02](docs/github-classroom-02.png)

3. Fill in the template **GitHub repository** `Compiler-s24/tmpl-hw0` -> **Continue**.
   <br>![github-classroom-03](docs/github-classroom-03.png)

4. Press **Create assignment**.
   <br>![github-classroom-04](docs/github-classroom-04.png)

5. Copy the url to `Homeworks/README.md` in repository `private`.
   <br>![github-classroom-05](docs/github-classroom-05.png)

6. Publish the announcement on the course website, and create the announcement issue in the `Discussion` repository.


### Grade homework
We retrieve late submission penalty via `collect-homework`, and grade students' homework via `grade-homework`. Take `hw1` in Compiler-s24 as an example:
- Deadline: 2024-03-21 23:59:59
- Penalty: 15% per day (weekends count as 1 day)
- Late submission duration: 3 days

(Retrieve penalty)

1. **Run command: `./hand collect-homework hw1 2024-03-21-23-59-59 3 hw1-penalty.txt`**
   <details>
   <summary>Output</summary>

   ```
   Retrieving repositories with prefix 'hw1'...
   Retrieving default branches...
   Retrieving pushes before deadline...

   [Summary]
       Total students: 49
       Total submissions: 42
       Late submissions: 0
       Deadline: 2024-03-21 23:59:59
       Cutoff: 2024-03-24 23:59:59
   ```

   </details>

2. hand2 retrieves the last push time before `2024-03-24 23:59:59` for students and calculate their penalties.

3. hand2 finds the sheet `hw1` in the spreadsheet.

4. hand2 iterates through the sheet and outputs a penalty for each student (100% if not submitted).

5. `hw1-penalty.txt` contains penalties in the same order as the sheet.

6. **Copy the penalties from `hw1-penalty.txt` to the sheet `hw1`.**

(Grade homework)

7. **Run command: `./hand grade-homework hw1 tmpl-hw1 solution 2024-03-21-23-59-59 3 hw1-score.txt`**
    - (*Note: If you are using WSL, launch the program in Docker via `docker/run.sh`. See details at [Using WSL](#using-wsl)*)
   <details>
   <summary>Output</summary>

   ```
   Retrieving homework template...
   Retrieving feedback repository...
   Retrieving repos with prefix 'hw1'...
   Retrieving default branches...
   Retrieving pushes before deadline...
   Retrieving student repositories...
   Retrieving homework from 'hw1-username-01'
   Retrieving homework from 'hw1-username-02'
   Retrieving homework from 'hw1-username-03'
   Retrieving homework from 'hw1-username-04'
   Retrieving homework from 'hw1-username-05'
   Retrieving homework from 'hw1-username-06'
   Retrieving homework from 'hw1-username-07'
   Retrieving homework from 'hw1-username-08'
   Retrieving homework from 'hw1-username-09'
   Retrieving homework from 'hw1-username-10'
   Retrieving homework from 'hw1-username-11'
   Retrieving homework from 'hw1-username-12'
   Retrieving homework from 'hw1-username-13'
   Retrieving homework from 'hw1-username-14'
   Retrieving homework from 'hw1-username-15'
   Retrieving homework from 'hw1-username-16'
   Retrieving homework from 'hw1-username-17'
   Retrieving homework from 'hw1-username-18'
   Retrieving homework from 'hw1-username-19'
   Retrieving homework from 'hw1-username-20'
   Retrieving homework from 'hw1-username-21'
   Retrieving homework from 'hw1-username-22'
   Retrieving homework from 'hw1-username-23'
   Retrieving homework from 'hw1-username-24'
   Retrieving homework from 'hw1-username-25'
   Retrieving homework from 'hw1-username-26'
   Retrieving homework from 'hw1-username-27'
   Retrieving homework from 'hw1-username-28'
   Retrieving homework from 'hw1-username-29'
   Retrieving homework from 'hw1-username-30'
   Retrieving homework from 'hw1-username-31'
   Retrieving homework from 'hw1-username-32'
   Retrieving homework from 'hw1-username-33'
   Retrieving homework from 'hw1-username-34'
   Retrieving homework from 'hw1-username-35'
   Retrieving homework from 'hw1-username-36'
   Retrieving homework from 'hw1-username-37'
   Retrieving homework from 'hw1-username-38'
   Retrieving homework from 'hw1-username-39'
   Retrieving homework from 'hw1-username-40'
   Retrieving homework from 'hw1-username-41'
   Retrieving homework from 'hw1-username-42'
   Start grading...
   [1/42] Grading '/hand/cache/hw1-username-01_7e69cfff0dcf7ac3d4c992536b4ee7b28a794e24'
   [2/42] Grading '/hand/cache/hw1-username-02_4f9add02ee44131bfc5d9d5e43015211d18f69ad'
   [3/42] Grading '/hand/cache/hw1-username-03_f98a5def30daa0b84199655e5a16a44a8d05da38'
   [4/42] Grading '/hand/cache/hw1-username-04_802da73a0da80e6513c70852134c21ae63dd1a6b'
   [5/42] Grading '/hand/cache/hw1-username-05_feb8a5dcc2291c5d01c6cba9616461d2ad4fd30d'
   [6/42] Grading '/hand/cache/hw1-username-06_b19649c3010c2fb1036815e880d61f8e98a4262b'
   [7/42] Grading '/hand/cache/hw1-username-07_7b86d55db032cd3522e5d1e8cdec3bfed6cc4506'
   [8/42] Grading '/hand/cache/hw1-username-08_007ac121f8f39292cab6e0b4ca5508446c88ca53'
   [9/42] Grading '/hand/cache/hw1-username-09_8c9ae90627babee59f9a95af6b042f3455204eda'
   [10/42] Grading '/hand/cache/hw1-username-10_6a5f3c09fe14658f7565684ddb4e2a77fd0ee2a8'
   [11/42] Grading '/hand/cache/hw1-username-11_3e69fe7b23241df3875c1465676f392907e1ea0d'
   [12/42] Grading '/hand/cache/hw1-username-12_fcb9a7e1f0f2942c21eab815c236819b58bcbd8e'
   [13/42] Grading '/hand/cache/hw1-username-13_ae871233c207e804f6ce89cedf9498510aa41714'
   [14/42] Grading '/hand/cache/hw1-username-14_b24d3f5e2b133cfeeadbefd3aac22a45892627ab'
   [15/42] Grading '/hand/cache/hw1-username-15_d99b42c210f72a9c0967c44fb028476279ee1194'
   [16/42] Grading '/hand/cache/hw1-username-16_da81d1f49c0b6b9abecc181c9276230abfe16d83'
   [17/42] Grading '/hand/cache/hw1-username-17_dd2899cd1c9a0121adca0f8d27af55c05344d632'
   [18/42] Grading '/hand/cache/hw1-username-18_41f9aa3931513e95ba42314597e8997fd5736cbc'
   [19/42] Grading '/hand/cache/hw1-username-19_c6a427b9e1538a680f8c5a1d30aedd21c44b6263'
   [20/42] Grading '/hand/cache/hw1-username-20_233bb7c236a8a174fbe36802b49e6729088050ca'
   [21/42] Grading '/hand/cache/hw1-username-21_91d4063b9ddaa2d52d24bf74f9aade0aba77e2cc'
   [22/42] Grading '/hand/cache/hw1-username-22_8eda15239786d1dc8d33c8f8181bdd9d468d2853'
   [23/42] Grading '/hand/cache/hw1-username-23_7b1d36d92ca1961d23464d477e5963ff507f54d2'
   [24/42] Grading '/hand/cache/hw1-username-24_6f6dd1d17460cc426ec74f8dbe04c560cc1f0fdc'
   [25/42] Grading '/hand/cache/hw1-username-25_7213c116cc5f53970e66011cbe2f9b86a5c6d53a'
   [26/42] Grading '/hand/cache/hw1-username-26_f2790783c7fb8e4bca150a66a6e07f2877cce4f9'
   [27/42] Grading '/hand/cache/hw1-username-27_815dd6986a68f6f5db1a1766d4b27153c5f2717a'
   [28/42] Grading '/hand/cache/hw1-username-28_2591b6cbbfcc457d9fd12617fa52d5587428f230'
   [29/42] Grading '/hand/cache/hw1-username-29_d77e72e523827cdfa8f77cdbeda513dcbda73515'
   [30/42] Grading '/hand/cache/hw1-username-30_055c0a014758ed6bea709026c8d1f697dfc92826'
   [31/42] Grading '/hand/cache/hw1-username-31_60fca180032002b881824ef1f86f8eb58a5c2fb2'
   [32/42] Grading '/hand/cache/hw1-username-32_cf4fefe89e33a49ce89885381ef0ff789633a15f'
   [33/42] Grading '/hand/cache/hw1-username-33_496c801038faa4259360894c0095d0644aa46bd4'
   [34/42] Grading '/hand/cache/hw1-username-34_5fed7405910366a485cce2b1f855dc72d50b7c50'
   [35/42] Grading '/hand/cache/hw1-username-35_90621d18ae6261bcdb055b6033ba149e09160712'
   [36/42] Grading '/hand/cache/hw1-username-36_e3edc4ee7b4d5117e094e4567375ba39aab794bf'
   [37/42] Grading '/hand/cache/hw1-username-37_5175652f741f55f26627b0d49d7658c7e4350f00'
   [38/42] Grading '/hand/cache/hw1-username-38_57b4fe084b86819c1392788ebcf4a73212fa3711'
   [39/42] Grading '/hand/cache/hw1-username-39_983894d9b5958c7b312bc63c3c5ba995e8a77033'
   [40/42] Grading '/hand/cache/hw1-username-40_eaf5fd17fc63f3872e5dbbfbb059a0113a3c52fa'
   [41/42] Grading '/hand/cache/hw1-username-41_6522b7bb6f7905dd7ac6991d1dd0147c12e07920'
   [42/42] Grading '/hand/cache/hw1-username-42_8737e0a1bd78c5ad1c6aca258d27c16d0e1ff3d0'
   Generating report...

   [Summary]
       Total students: 49
       Total submissions: 42
       Late submissions: 0
       Failed submissions: 0
       Deadline: 2024-03-21 23:59:59
       Cutoff: 2024-03-24 23:59:59
   ```

   </details>

8. hand2 retrieves the last push time before `2024-03-24 23:59:59` for students and calculate their penalties.

9. hand2 finds the sheet `hw1` in the spreadsheet.

10. hand2 clones the repository `hw-feedback` and the branch `solution` in `tmpl-hw1`.

11. hand2 finds repositories with the prefix `hw1` in `Compiler-s24`, and clones their last commits before the cutoff.

10. hand2 replaces the directories `./docker` and `./test` in students' repositories with those in `tmpl-hw1`.

11. hand2 iterates through students' repositories and executes the [grade command](#grade_command). Multiple [grading threads](#grade_thread) are launched, with each thread grading one student's repository simultaneously.

12. hand2 iterates through the sheet, performs the following actions for each student:

    1. If the repository doesn't exist, output a score of 0 and skip the remaining actions.

    2. Generate feedback `./hw1/reports/<student_id>.md`. The file replacement is performed at this stage, see details at [Feedback format](#feedback_format).

    3. Parse the first number in the [score file](#score_relative_path) and output it to `hw1-score.txt` as the score for the student.

13. hand2 commits and pushes `hw-feedback` to GitHub (the committer is retrieved from [github_access_token](#github_access_token) and the [email](#email) is from the configuration).
    - Use the flag `--dry` to skip this step, you will commit and push the feedbacks yourself. For example: `./hand grade-homework --dry hw1 tmpl-hw1 solution 2024-03-21-23-59-59 3 hw1-score.txt`

14. **Copy the scores from `hw1-score.txt` to the sheet `hw1`.**


### Announce grade
Open an issue in every student's repository as the grade announcement, the content is derived from `hw-feedback` and the *homework sheet*. Take `hw1` in Compiler-s24 as an example:

1. **Run command: `./hand announce-grade hw1`**
   <details>
   <summary>Output</summary>

   ```
   Retrieving feedback repository...
   Retrieving repos with prefix 'hw1'...
   Retrieving issue 'Grade for hw1'...
   Announce grade for 'hw1-username-01'
   Announce grade for 'hw1-username-02'
   Announce grade for 'hw1-username-03'
   Announce grade for 'hw1-username-04'
   Announce grade for 'hw1-username-05'
   Announce grade for 'hw1-username-06'
   Announce grade for 'hw1-username-07'
   Announce grade for 'hw1-username-08'
   Announce grade for 'hw1-username-09'
   Announce grade for 'hw1-username-10'
   Announce grade for 'hw1-username-11'
   Announce grade for 'hw1-username-12'
   Announce grade for 'hw1-username-13'
   Announce grade for 'hw1-username-14'
   Announce grade for 'hw1-username-15'
   Announce grade for 'hw1-username-16'
   Announce grade for 'hw1-username-17'
   Announce grade for 'hw1-username-18'
   Announce grade for 'hw1-username-19'
   Announce grade for 'hw1-username-20'
   Announce grade for 'hw1-username-21'
   Announce grade for 'hw1-username-22'
   Announce grade for 'hw1-username-23'
   Announce grade for 'hw1-username-24'
   Announce grade for 'hw1-username-25'
   Announce grade for 'hw1-username-26'
   Announce grade for 'hw1-username-27'
   Announce grade for 'hw1-username-28'
   Announce grade for 'hw1-username-29'
   Announce grade for 'hw1-username-30'
   Announce grade for 'hw1-username-31'
   Announce grade for 'hw1-username-32'
   Announce grade for 'hw1-username-33'
   Announce grade for 'hw1-username-34'
   Announce grade for 'hw1-username-35'
   Announce grade for 'hw1-username-36'
   Announce grade for 'hw1-username-37'
   Announce grade for 'hw1-username-38'
   Announce grade for 'hw1-username-39'
   Announce grade for 'hw1-username-40'
   Announce grade for 'hw1-username-41'
   Announce grade for 'hw1-username-42'

   [Summary]
       Total students: 49
       Total announcements: 42
       Failed announcements: 0
   ```

   </details>

2. hand2 clones the repository `hw-feedback`.

3. hand2 finds the sheet `hw1` in the spredsheet.

4. hand2 generates issue contents for each student, derived from `hw-feedback/hw1/reports/<student_id>.md` and the sheet `hw1`. The sheet replacements are performed at this stage, see details at [Feedback format](#feedback-format).

5. hand2 iterates through the sheet, and performs the following actions for each student's repository:

    1. If an issue named `Grade for hw1` exists, update it to match the issue content; otherwise, create a new issue.
    - Use the flag `--dry` to output the issue content without actually editing or creating the issue.

### Clean
Delete unnecessary files, such as logs and caches.

1. **Run command: `./hand clean`**
   <details>
   <summary>Output</summary>

   ```
   Deleting cache from '/hand/cache'...
   Deleting log from '/hand/log'...
   ```

   </details>


## Explanation

### Course structure
The course includes a GitHub organization and a Google spreadsheet.

The GitHub organization structure:
```
<organization>
│
├─ repository
│  │
│  ├─ <feedback-repository>
│  ├─ tmpl-hw1
│  ├─ tmpl-hw2
│  └─ ...
│
└─ team
   ├─ <ta-team>
   └─ <student-team>
```
- `<organization>`: GitHub organization for the course. Students join this organization to receive homework, submit homework, and access the discussion forum.
- `tmpl-hwX`: Templates for homework hwX. Students create their homework repository from these templates and submit their homework through commits.
- `<feedback-repository>`: Usually named `hw-feedback`, it stores feedbacks to students before announcing grades. See details at [Feedback format](#feedback-format).
- `<...>`: Other repositories, such as the course website, are not used by hand2.
- `<ta-team>`: Team for the professor and teaching assistants, it has administrative privileges over the entire organization.
- `<student-team>`: Team for students, the members of ta-team can also join it.

The Google spreadsheet structure:
```
<spreadsheet>
│
└─ sheet
   │
   ├─ hw1
   ├─ hw2
   └─ ...
```
- `<spreadsheet>`: Google spreadsheet for organizing students' grades. it must be visible to the public for hand2 to work.
    - (*Note: hand2 doesn't require write access, and will not write to the spreadsheet.*)
  <details>
  <summary>Spreadsheet example</summary>
  
  ![google-spreadsheet-01](docs/google-spreadsheet-01.png)
  
  </details>
- `hwX`: *Homework sheet* for managing grades for hwX. The first row of the sheet is called `label`. A valid *homework sheet* must contain the following labels:
    - students' GitHub username, see also [username_label](#username_label).
    - students' IDs, see also [student_id_label](#student_id_label).
- `<...>`: Other sheets, such as student info, are not used by hand2.


### Timezone
Timezone is always UTC+8.


### Deadline and Cutoff
A deadline is the start of late submission, and a cutoff is the end of late submission. They are often passed in as `deadline` and `cutoff_time`.
- `deadline`: the time when late submission begins.
- `cutoff_time`: the duration (in days) from the start of late submission to its conclusion.

For example, the late submission of `hw1` is 2024-03-21 23:59:59 -> 2024-03-24 23:59:59. Hence, `deadline = 2024-03-21-23-59-59` and `cutoff_time = 3`.


### Feedback format
A feedback follows two stages of transformation before it's delivered to a student's issue. Take `hw1` in Compiler-s24 as an example,

1. Initially, a feedback template `./report_template_hw1.md` is located in the repository `hw-feedback`.
   <details>
   <summary>hw-feedback/report_template_hw1.md</summary>

   ````
   Hi, @${github_handle}

   ### Information

   - Grade: ${grade}
   	- testcase : ${grade_testcase}/100 pts
   	- report : ${grade_report}/5 pts
   	- penalty: ${penalty}
   - Grading rule:
   	- penalty x (testcase * 100% + report)
   - Grader: ${grader}

   ### Output Diff

   <details>
     <summary>Click to expand!</summary>

   ```
   ${./test/result/diff.txt}
   ```

   </details>

   ### Note

   If you got any questions about this, please make a comment stating your arguments below and tag the grader in the comment.
   Any request after the deadline will be ignored.
   ````

   </details>

2. Stage 1: After grading homework, hand2 replaces `${./<...>}` placeholders in the template with corresponding files within the student's repository.
   - *(Note: If the file doesn't exist, it will be replaced with an empty string.)*

   For example: the student's repository contains `./test/diff.txt`:
   ```
   abcdef

   ```
   The transformed feedbacks are output to `hw-feedback/hw1/reports`:
   <details>
   <summary>hw-feedback/hw1/reports/&lt;student_id&gt;.md</summary>

   ````
   Hi, @${github_handle}

   ### Information

   - Grade: ${grade}
   	- testcase : ${grade_testcase}/100 pts
   	- report : ${grade_report}/5 pts
   	- penalty: ${penalty}
   - Grading rule:
   	- penalty x (testcase * 100% + report)
   - Grader: ${grader}

   ### Output Diff

   <details>
     <summary>Click to expand!</summary>

   ```
   abcdef

   ```

   </details>

   ### Note

   If you got any questions about this, please make a comment stating your arguments below and tag the grader in the comment.
   Any request after the deadline will be ignored.
   ````
   </details>

3. Stage 2: When announcing grade, hand2 replaces `${<...>}` placeholders in `hw-feedback` with corresponding data from the sheet `hw1`.
   - *(Note: If the label doesn't exist in the sheet, it will result in an error.)*

   For example, we are announcing for `username-01`, and the sheet has the form:
   <br>![google-spreadsheet-03](docs/google-spreadsheet-03.png)

   The transformed feedbacks are used as issue content for grade announcements.
   <details>
   <summary>Issue: Grade for hw1</summary>

   ````
   Hi, @username-1

   ### Information

   - Grade: 105
   	- testcase : 100/100 pts
   	- report : 5/5 pts
   	- penalty: 100%
   - Grading rule:
   	- penalty x (testcase * 100% + report)
   - Grader: @Compiler-s24/TA

   ### Output Diff

   <details>
     <summary>Click to expand!</summary>

   ```
   abcdef

   ```

   </details>

   ### Note

   If you got any questions about this, please make a comment stating your arguments below and tag the grader in the comment.
   Any request after the deadline will be ignored.
   ````

   </details>


### Logging
Enable logging with the flag `--log` for any command. Logs will be saved to `./log/YYYY-MM-DD_hh-mm-ss.log`. For example: `./hand --log config-check`


### Using WSL
- Running a Docker container on WSL can occasionally mess up the `inode` of `..` that points to `/mnt`. This problem is observed in development, and it consistently causes failure when trying to query the full path, which is crucial for both the homework make scripts and hand2.
- Run command `docker/run.sh ...` to use the Docker version of hand2 in WSL instead. The Docker scripts support Docker in Docker (DinD), allowing you to grade students' homework even if the grading script also launches its own Docker container.


## How to build

### Windows
1. Run `build.bat` under `code\` to build the program on Windows. By default, it searches for MSVC, GCC, and clang, and uses the first compiler it finds.

2. To build with MSVC, run `build.bat` under MSVC x64 native tools command prompt, you can use `vcvarsall.bat x64` to launch the developer prompt in x64 mode. See also [Visual Studio Developer Command Prompt and Developer PowerShell](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell) and [How to: Enable a 64-Bit, x64 hosted MSVC toolset on the command line](https://learn.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line).
3. To build with GCC or clang, ensure GCC or clang is added to the `PATH` environment variable.
4. The built program can be found under `build\`.

### Linux
1. Run `./build.sh` under `code/` to build the program on Linux. By default, it searches for GCC and clang, and uses the first compiler it finds.
2. To build with GCC or clang, ensure GCC or clang is added to the `PATH` environment variable.
3. The built program can be found under `build/`.

### Docker
1. Run `docker/build.sh` to build the program with the Linux build script inside Docker.
2. The built program can be found under `build/`.
3. Run `docker/run.sh` [args] ...` to execute the program inside Docker.
4. To debug hand2 with GDB inside Docker, run `docker/debug.sh [args] ...`.
5. Any calls to `build.sh`, `run.sh` or `debug.sh` automatically build the Docker image if needed.


## How to build the dependency
The dependencies of Windows and Linux are pre-built and placed within `code/`. If you only need to modify the program's source code, you can safely skip this section. However, in case you wish to build the dependencies yourself, here is how the dependencies are built:

### Windows
1. Run `win32_external\build.bat` to build and install the required dependencies on Windows. You must run under MSVC x64 native tools command prompt.
2. To automatically download and extract the dependencies, `curl` and `tar` are required in your `PATH`. Alternatively, you can manually download the dependencies, the URL and required directory structure will displayed by `build.bat` if the download fails.
3. The include files and libraries are placed in `code\include_win32` and `code\lib_win32`, respectively.
4. `win32_external\deps` stores the source files and the build output for the dependencies. You can safely delete it after the build process.
5. `win32_external\log` stores logs during the build process. To see how logs are generated, look into `win32_external\build.bat`.

### Linux
1. Linux dependencies are built using Docker. Run `linux_external/build.sh` to build and install the required dependencies on Linux.
2. The include files and libraries are placed in `code\include_linux` and `code\lib_linux`, respectively.
3. `linux_external/log` stores logs during the build process. To see how logs are generated, look into `linux_external/Dockerfile`.


## How to publish
Before publishing the program as an archive to [release](https://github.com/Compiler-s24/hand2/releases), ensure that you have built both the Windows and Linux programs. The archive will include Windows, Linux, and Docker versions of the program.

The published archive remains uncompressed for Windows, and will only be compressed for Linux if `zip` is available. Consider using the Docker script to ensure the files are compressed.

### Windows
1. Run `publish.bat [version]` under `code\` to generate the archive.
2. The archive `hand2-v[version].zip` can be found under `publish\`.
3. Upload the archive to GitHub.

### Linux
1. Run `./publish.sh [version]` under `code/` to generate the archive.
2. The archive `hand2-v[version].zip` can be found under `publish/`.
3. Upload the archive to GitHub.

### Docker
1. Run `docker/publish.sh [version]` to generate the archive.
2. The archive `hand2-v[version].zip` can be found under `publish/`.
3. Upload the archive to GitHub.
