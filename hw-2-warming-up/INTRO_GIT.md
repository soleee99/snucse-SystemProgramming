[[_TOC_]]

# Introduction to Git

Git is a distributed code version-control system. A version-control system allows you to track your changes in the software development cycle. It is good practice to commit your changes to the version-control system often so that (a) you have a history of how your code evolved and (b) you can restore it in case you lose the data on your local disk. 

Git is a very powerful version-control system, actually too powerful for our purposes. However, Git comes with by far the best web-interface (Gitlab which runs on teaching.csap.snu.ac.kr) and is used by hundreds of thousands of projects, so knowing your way around Git is essential for any software developer these days.

We provide a very crude introduction to Git and only describe the most basic workcycle. For details about Git, refer to the official [Git documentation](https://git-scm.com/docs).

The basic work cycle of a Git project is *clone*, *edit*, *stage*, *commit*, *push*. The project only needs to *clone*d once, after that, the edit cycle (*edit*, *stage*, *commit*) is repeated several times. Whenever you want to upload your commited changes back to the Git server, you can *push* the local changes. If you have copies of the same project on several computers, you can download the most recent version by *pull*ing it from the server. This is also necessary when several people work on a project.

## Clone
After you create (or fork) a project on Gitlab (or Github), you first need to download the project to your local disk. This is achieved by clicking on the 'Clone' button in your project overview. Copy the address shown below 'Clone with HTTPS'.

In your VM, open a terminal and go to the director under which you want to store the project. In this example, we are going to create a folder 'homework' in our shared folder and place the Git project in there.

```bash
$ mkdir -p ~/share/homework
$ cd ~/share/homework
```
Now it's time to clone the project to your local disk. Copy-paste the address you copied in the webinterface before. After entering your username and password, the project is cloned to your disk. Enter the project folder.
```bash
$ git clone https://teaching.csap.snu.ac.kr/yourname/hw-2-warming-up.git
Cloning into 'hw-2-warming-up'...
Username for 'https://teaching.csap.snu.ac.kr': <your@email.com>
Password for 'https://bernhard@csap.snu.ac.kr@teaching.csap.snu.ac.kr': <your password>
remote: Enumerating objects: 9, done.
remote: Counting objects: 100% (9/9), done.
remote: Compressing objects: 100% (6/6), done.
remote: Total 9 (delta 0), reused 0 (delta 0), pack-reused 0
Unpacking objects: 100% (9/9), done.
$ ls
hw-2-warming-up
$ cd hw-2-warming-up
$ ls
hw2.reference  main.c  Makefile  README.md
```

## Edit
You can now edit files at will. At any time, you can check the status of your Git repository using the *status* command.

```bash
$ vi main.c 
$ vi mathlib.h
$ git status
On branch master
Your branch is up-to-date with 'origin/master'.
Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git checkout -- <file>..." to discard changes in working directory)

	modified:   main.c

Untracked files:
  (use "git add <file>..." to include in what will be committed)

	mathlib.h

no changes added to commit (use "git add" and/or "git commit -a")

```
We observe that main.c which is part of the repository has been modified. A new file called *newfile.c* exists, but is not being tracked by Git.

## Stage and Commit

To store your modifications locally on your disk, you need to first *stage* and then *commit* your work. Unlike almost all other code-versioning systems, Git does not automatically include modified files; instead, you have to add them explicitly every time you commit. New (untracked) files need to be added to the repository before Git cares about them.

We continue with the example above where we have modified *main.c* and created a new file *mathlib.h*. We want to add both the modified file as the new file and commit our changes. First, we need to add these files to the so-called staging area which is basically a list of files that are to be committed on the next commit.
```bash
$ git add main.c mathlib.h
$ git status
On branch master
Your branch is up-to-date with 'origin/master'.
Changes to be committed:
  (use "git reset HEAD <file>..." to unstage)

	modified:   main.c
	new file:   mathlib.h
```

To commit (save) the staged files to the local repository, execute the *commit* command.
```bash
$ git commit -m 
"This comment describes my changes."
[master ce497ec] This comment describes my changes.
 2 files changed, 1 insertion(+)
 create mode 100644 mathlib.h
```

Now your repository does not contain any modified files anymore, but there is one commit that you haven't *push*ed (uploaded) to the server yet.
```bash
$ git status
On branch master
Your branch is ahead of 'origin/master' by 1 commit.
  (use "git push" to publish your local commits)
nothing to commit, working tree clean
```

You don't need to upload every single commit to the server, nevertheless, it is good practice to push your changes from time to time - only then will you have a backup copy on the server in case your disk crashes.

## Push and Pull

Push uploaded all new commits to the server. Pull does the opposite: it downloads the newest version from the remote repository. 

To push our change from before, simply execute.
```bash
$ git push
Counting objects: 4, done.
Delta compression using up to 4 threads.
Compressing objects: 100% (3/3), done.
Writing objects: 100% (4/4), 379 bytes | 0 bytes/s, done.
Total 4 (delta 2), reused 0 (delta 0)
To https://teaching.csap.snu.ac.kr/yourname/hw-2-warming-up.git
   ec3fa02..ce497ec  master -> master
```

If you have configured Git as recommended in Lab 1 - The Basics, your password should still be cached and you won't have to enter it again.

To pull new changes from the remote server, issue the *pull* comment.
```bash
$ git pull
git pull
Already up-to-date.
```
