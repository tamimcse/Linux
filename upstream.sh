git remote add davem git://git.kernel.org/pub/scm/linux/kernel/git/davem/net-next.git
git fetch davem master
git branch -r
git fetch --tags davem
#git tag
git merge v4.14
#Merge main davem's main branch
#git merge davem/master

