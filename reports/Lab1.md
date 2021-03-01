## Lab 1 stitching substrings into a byte stream

### 1 Getting start

由于后面要用的代码在lab1-startercode下，若并不是git clone官方仓库，则需要新增一个远程仓库

```
git remote add raw https://github.com/CS144/sponge.git
```

将新仓库数据抓取到本仓库

```
git fetch raw
...
From https://github.com/CS144/sponge
 * [new branch]      lab1-startercode -> raw/lab1-startercode
 * [new branch]      lab2-startercode -> raw/lab2-startercode
 * [new branch]      lab3-startercode -> raw/lab3-startercode
 * [new branch]      lab4-startercode -> raw/lab4-startercode
 * [new branch]      lab5-startercode -> raw/lab5-startercode
 * [new branch]      lab6-startercode -> raw/lab6-startercode
 * [new branch]      lab7-startercode -> raw/lab7-startercode
 * [new branch]      master           -> raw/master
```

利用新仓库的分支，在本地新建分支，并将main分支合并过来

```
git checkout -b lab1-startcode raw/lab1-startercode
git merge main
```

在新的分支下git push，即可在origin仓库创建对应分支

```
git checkout lab1-startcode
git commit -m "preparing for lab1"
git push origin lab1-startcode
git branch -r
    origin/HEAD -> origin/main
    origin/lab1-startcode
    origin/main
    raw/lab1-startercode
    raw/lab2-startercode
    raw/lab3-startercode
    raw/lab4-startercode
    raw/lab5-startercode
    raw/lab6-startercode
    raw/lab7-startercode
    raw/master
```



### 2 实验过程

主要流程有：容量判断，处理字串的冗余前缀，合并字串，写入字节流，EOF判断