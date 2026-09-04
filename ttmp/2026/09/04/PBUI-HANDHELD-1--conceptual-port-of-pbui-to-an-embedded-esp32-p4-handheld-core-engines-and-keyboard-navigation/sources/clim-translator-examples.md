[![Next](../next.gif)](clim-ch8-6.htm) [![Prev](../prev.gif)](clim-ch8-4.htm) [![Up](../up.gif)](clim-ch8.htm) [![Top](../top.gif)](clim.htm) [![Contents](../contents.gif)](clim-contents.htm#clim-ch8-5) [![Index](../index.gif)](clim-index.htm)

### 8.5 Examples of Defining Presentation Translators in CLIM

#### 8.5.1 Defining a Translation from Floating Point Number to Integer

Here is an example that defines a presentation translator to accept an [integer](http://www.lispworks.com/documentation/HyperSpec/Body/t_intege.htm) object from a [float](http://www.lispworks.com/documentation/HyperSpec/Body/a_float.htm) presentation. Users have the options of typing in a float or integer to the input prompt or clicking on any [float](http://www.lispworks.com/documentation/HyperSpec/Body/a_float.htm) or [integer](http://www.lispworks.com/documentation/HyperSpec/Body/t_intege.htm) presentation.

```
(define-presentation-translator integer-to-float
  (integer float my-command-table
           :documentation "Integer as float"
           :gesture :select
           :tester ((object) (integerp object))
           :tester-definitive t)
  (object)
  (float object))

(clim:present most-positive-fixnum)

(clim:accept 'float)
```

#### 8.5.2 Defining a Presentation-to-Command Translator

The following example defines the [delete-file](http://www.lispworks.com/documentation/HyperSpec/Body/f_del_fi.htm) presentation-to-command translator:

```
(clim:define-presentation-to-command-translator
 delete-file
 (pathname com-delete-file my-command-table
           :documentation "Delete this file"
           :gesture :delete)
 (object)
 (list object))
```

#### 8.5.3 Defining Presentation Translators for the Blank Area

You can also write presentation translators that apply to blank areas of the window, that is, areas where there are no presentations. Use [blank-area](clim-ch6-5.htm#CLIM\)blank-area) as the from-presentation-type. There is no highlighting when such a translator is applicable, since there is no presentation to highlight. You can write presentation translators that apply in any context by supplying `nil` as the to-presentation-type.

When you are writing an interactive graphics routine, you will probably encounter the need to have commands available when the mouse is not over any object. To do this, you write a *translator* from the blank area.

The presentation type of the blank area is [blank-area](clim-ch6-5.htm#CLIM\)blank-area). You probably want the :x and :y arguments to the translator.

For example:

```
(clim:define-presentation-to-command-translator
 add-circle-here
 (clim:blank-area com-add-circle my-command-table
                  :documentation "Add a circle here.")
 (x y)
 \`(,x ,y))
```

#### 8.5.4 Defining a Presentation Action

Presentation actions are only rarely needed. Often a presentation-to-command translator is more appropriate. One example where actions are appropriate is when you wish to pop up a menu during command input. Here is how CLIM's general menu action could be implemented:

```
(clim:define-presentation-action
 presentation-menu
 (t nil clim:global-command-table
    :tester-definitive t :documentation "Menu"
    :menu nil :gesture :menu)
 (presentation frame window x y)
 (clim:call-presentation-menu presentation clim:*input-context*
                              frame window x y :for-menu t))
```

---

*CLIM 2.0 User Guide - 01 Dec 2021 19:38:57*


[![Next](../next.gif)](clim-ch8-6.htm) [![Prev](../prev.gif)](clim-ch8-4.htm) [![Up](../up.gif)](clim-ch8.htm) [![Top](../top.gif)](clim.htm) [![Contents](../contents.gif)](clim-contents.htm#clim-ch8-5) [![Index](../index.gif)](clim-index.htm)
