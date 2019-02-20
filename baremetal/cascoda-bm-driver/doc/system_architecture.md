# Baremetal system architecture
This project is the core of any baremetal system/application using the ca821x-api library (and consequently the CA-8210 device). There are still some components missing which must be integrated however.
## Port/vendor code
These baremetal drivers are designed to be generic and portable to any baremetal platform, and as such there is additional support needed for any particular MCU/eval board. This usually takes the form of a *port* directory and a *vendor* directory at the same level as the *cascoda-bm-driver* directory:
```
cascoda-vendor-platform/
|
|-> cascoda-bm-driver/
|   |
|   |-> ca821x-api/
|   |-> source/
|   |-> include/
|-> port/
|   |
|   |-> source/
|   |-> include/
|-> vendor/
```
The functions in the port directory should have consistent prototypes, as expected by the cascoda-bm-driver code. The definitions of these functions however is completely dependent on what is provided in the vendor directory.

The other component required is of course one or more applications:

```
cascoda-vendor-platform/
|
|-> apps/
|   |
|   |-> app1/
|   |   |-> source/
|   |   |-> include/
|   |-> app2/
|   |   |-> source/
|   |   |-> include/
|   |-> app3/
|   |   |-> source/
|   |   |-> include/
|-> cascoda-bm-driver/
|   |
|   |-> ca821x-api/
|   |-> source/
|   |-> include/
|-> port/
|   |
|   |-> source/
|   |-> include/
|-> vendor/
```
Each application must provide a **main** function, as well as implement the requirements of both the ca821x-api and cascoda-bm-driver projects. These requirements are detailed in the README files of each.

All of this is typically wrapped up in a root directory (and ideally repository) with the following naming convention:
```
cascoda-<vendor name>-<platform name>
```
For example, for the vendor "Microcorp Limited" who produce a platform "Superboard 2000" it might be:
```
cascoda-microcorp-super2k
```
