#pragma warning disable CS1591
using System;
using System.Xml;
using System.Globalization;
public class runme
{
  
  static void Main()
  {
    runme r = new runme();
    r.run();
  }
  
  void run()
  {
    using (XmlReader xmlReader = XmlReader.Create("doxygen_basic_translate.xml"))
    {
      while (xmlReader.Read())
      {
        if (xmlReader.NodeType == XmlNodeType.Element && xmlReader.Name == "member")
        {
          string raw_name = xmlReader["name"];
          string raw_xml = xmlReader.ReadInnerXml();
          
          switch(raw_name)
          {
            case "M:doxygen_basic_translate.function":
              compareComment(raw_xml, @"
            <summary>
             
            Brief description. 
             
            The comment text.
            </summary> 
            <author>Some author</author> 
            <returns>Some number</returns> 
            <seealso cref=""M:doxygen_basic_translate.function2"" />
        ");
              break;
            case "M:doxygen_basic_translate.function2":
              compareComment(raw_xml, @"
            <summary>
            A test of a very very very very very very very very very very very very very very very very 
            very very very very very long comment string.
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function3(System.Int32)":
              compareComment(raw_xml, @"
            <summary>
            A test for overloaded functions 
            This is function <b>one</b>
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function3(System.Int32,System.Int32)":
              compareComment(raw_xml, @"
            <summary>
            A test for overloaded functions 
            This is function <b>two</b>
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function4":
              compareComment(raw_xml, @"
            <summary>
            A test of some mixed tag usage
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function5(System.Int32)":
              compareComment(raw_xml, @"
            <summary>
             This is a post comment. 
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function6(System.Int32)":
              compareComment(raw_xml, @"
            <summary>
            Test for default args
            </summary> 
            <param name=""a""> Some parameter, default is 42</param>
        ");
              break;
            case "M:doxygen_basic_translate.function6":
              compareComment(raw_xml, @"
            <summary>
            Test for default args
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function7(SWIGTYPE_p_p_p_Shape)":
              compareComment(raw_xml, @"
            <summary>
            Test for a parameter with difficult type 
            (mostly for python)
            </summary> 
            <param name=""a""> Very strange param</param>
        ");
              break;
            case "M:doxygen_basic_translate.Atan2(System.Double,System.Double)":
              compareComment(raw_xml, @"
            <summary>
                Multiple parameters test. 
             
                
            </summary><param name=""y""> Vertical coordinate. 
                </param><param name=""x""> Horizontal coordinate. 
                </param><returns>Arc tangent of <code>y/x</code>.</returns>
        ");
              break;
            case "M:doxygen_basic_translate.function8":
              compareComment(raw_xml, @"
            <summary>
            Test variadic function
            </summary>
        ");
              break;
            case "M:doxygen_basic_translate.function9(System.Int32)":
              compareComment(raw_xml, @"
            <summary>
            Test unnamed argument
            </summary>
        ");
              break;
          }
        }
      }
    }
  }

  void compareComment(string actual, string expected)
  {
        // remove \r, to avoid \n / \r\n confusion
        actual = actual.Replace("\r", String.Empty);
        expected = expected.Replace("\r", String.Empty);
        if (actual != expected)
        {
            throw new SystemException("Comments don't match. Actual: " + actual + "\n Expected: " + expected);
        }
    }
}