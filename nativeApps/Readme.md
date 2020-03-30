
# Steps to creat new naitve application
The following are the steps required to generate a native application for firebolt. This steps generate a helloworld sample app
1. Implement a sample application. Use C or C++ source code. (helloworld.cpp)
2. Add the sample application to the samples folder in the Firebolt SDK
2. source the environment (source RNE-3.0/environment-setup-cortexa7t2hf-vfp-vfpv4-neon-rdk-linux-gnueabi)
3. Compile the source ($CXX helloworld.cpp -o helloworld)
4. Create partnerapps folder, the folder structure should look like this
        - hworldapp <folder>
        - appmanagerregistry.conf
        - add the helloworld app to hworldapp folder
5. Update appmanagerregistry.conf file with helloworld app as shown,
 ``` 
                {"applications":
                  [
                    {
                      "displayName" : "helloworld sample",
                      "cmdName" : "helloworld",
                      "uri" : "/usb/partnerapps/hworldapp/helloworld",
                      "applicationType" : "native",
                      "version" : "1.0"
                    }
                  ]
                }
  ```
5. Copy the partnerapps folder to the root of usb drive (it can be fat,fat32,ext/2/3/4)
6. Connect usb to the Rapberry-Pi and press CTRL-E to refresh the firebolt home page
7. The new app should show up in the screen.
8. Use Cmdline to run the "helloWord" app.
9. To launch the app from UI, update the app to create display.
10. Refer the additional sample apps in the Firebolt SDK samples folder.
### Note
  For complex applications, autotools is supported. Please refer the sample applications for more details.
