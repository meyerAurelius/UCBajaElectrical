# UCBajaElectrical
A repo for the electrical subteam 2024-2025

**How to use this repo:**

**Installation: Debian/Ubuntu**
1. Create a Github account
2. ```sudo apt update && sudo apt install git ssh``` (ensures git and ssh are installed)
3. ```ssh-keygen -t ed25519 -C "your_email@example.com"``` (replace with your credentials used to create github account)
4. ```cat .ssh/id_ed25519.pub``` (copy this output)
5. Paste the output in step 4 into the "Key" section of this form: https://github.com/settings/ssh/new
6. Click "Add SSH Key" (fill in the title with bajaSshKey), this should succeed without errors
7. ```ssh-add .ssh/id_ed25519``` (makes the key you generated in step 3 availible to ssh)
8. Check the connection to github with ```ssh -T git@github.com``` (you should see your username in the output)
9. Change to a directory where you want the baja code stored
10. ```git clone https://github.com/SunStone7-Dragonnacho/UCBajaElectrical.git``` (clones the repo)
11. ```git remote set-url origin git@github.com:SunStone7-Dragonnacho/UCBajaElectrical.git``` (sets to origin to our repo for when code is pushed).
12. Enjoy!

**Installation: Windows**



**How to sync local code with code on github:**

1.```git pull```


**How to upload your work to git:**
1. Navigate to directory containing repo.
2. ```git add -A``` (adds all files to commit)
3. ```git commit -am "Label your commit withing these quotes"``` (creates a commit)
4. ```git push``` (pushes your commit to github)

